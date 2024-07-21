static int vrend_renderer_transfer_write_iov(struct vrend_context *ctx,
                                             struct vrend_resource *res,
                                             const struct iovec *iov, int num_iovs,
                                             const struct vrend_transfer_info *info)
{
   void *data;

   if ((is_only_bit(res->storage_bits, VREND_STORAGE_GUEST_MEMORY) ||
       has_bit(res->storage_bits, VREND_STORAGE_HOST_SYSTEM_MEMORY)) && res->iov) {
      return vrend_copy_iovec(iov, num_iovs, info->offset,
                              res->iov, res->num_iovs, info->box->x,
                              info->box->width, res->ptr);
   }

   if (has_bit(res->storage_bits, VREND_STORAGE_HOST_SYSTEM_MEMORY)) {
      assert(!res->iov);
      vrend_read_from_iovec(iov, num_iovs, info->offset,
                            res->ptr + info->box->x, info->box->width);
      return 0;
   }

   if (has_bit(res->storage_bits, VREND_STORAGE_GL_BUFFER)) {
      GLuint map_flags = GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_WRITE_BIT;
      struct virgl_sub_upload_data d;
      d.box = info->box;
      d.target = res->target;

      if (!info->synchronized)
         map_flags |= GL_MAP_UNSYNCHRONIZED_BIT;

      glBindBufferARB(res->target, res->id);
      data = glMapBufferRange(res->target, info->box->x, info->box->width, map_flags);
      if (data == NULL) {
	 vrend_printf("map failed for element buffer\n");
	 vrend_read_from_iovec_cb(iov, num_iovs, info->offset, info->box->width, &iov_buffer_upload, &d);
      } else {
	 vrend_read_from_iovec(iov, num_iovs, info->offset, data, info->box->width);
	 glUnmapBuffer(res->target);
      }
      glBindBufferARB(res->target, 0);
   } else {
      GLenum glformat;
      GLenum gltype;
      int need_temp = 0;
      int elsize = util_format_get_blocksize(res->base.format);
      int x = 0, y = 0;
      bool compressed;
      bool invert = false;
      float depth_scale;
      GLuint send_size = 0;
      uint32_t stride = info->stride;
      uint32_t layer_stride = info->layer_stride;

      if (ctx)
         vrend_use_program(ctx->sub, 0);
      else
         glUseProgram(0);

      if (!stride)
         stride = util_format_get_nblocksx(res->base.format, u_minify(res->base.width0, info->level)) * elsize;

      if (!layer_stride)
         layer_stride = util_format_get_2d_size(res->base.format, stride,
                                                u_minify(res->base.height0, info->level));

      compressed = util_format_is_compressed(res->base.format);
      if (num_iovs > 1 || compressed) {
         need_temp = true;
      }

      if (vrend_state.use_gles && vrend_format_is_bgra(res->base.format) &&
          !vrend_resource_is_emulated_bgra(res))
          need_temp = true;

      if (vrend_state.use_core_profile == true &&
          (res->y_0_top || (res->base.format == VIRGL_FORMAT_Z24X8_UNORM))) {
         need_temp = true;
         if (res->y_0_top)
            invert = true;
      }

      send_size = util_format_get_nblocks(res->base.format, info->box->width,
                                          info->box->height) * elsize;
      if (res->target == GL_TEXTURE_3D ||
          res->target == GL_TEXTURE_2D_ARRAY ||
          res->target == GL_TEXTURE_CUBE_MAP_ARRAY)
          send_size *= info->box->depth;

      if (need_temp) {
         data = malloc(send_size);
         if (!data)
            return ENOMEM;
         read_transfer_data(iov, num_iovs, data, res->base.format, info->offset,
                            stride, layer_stride, info->box, invert);
      } else {
         if (send_size > iov[0].iov_len - info->offset)
            return EINVAL;
         data = (char*)iov[0].iov_base + info->offset;
      }

      if (!need_temp) {
         assert(stride);
         glPixelStorei(GL_UNPACK_ROW_LENGTH, stride / elsize);
         glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, layer_stride / stride);
      } else
         glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

      switch (elsize) {
      case 1:
      case 3:
         glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
         break;
      case 2:
      case 6:
         glPixelStorei(GL_UNPACK_ALIGNMENT, 2);
         break;
      case 4:
      default:
         glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
         break;
      case 8:
         glPixelStorei(GL_UNPACK_ALIGNMENT, 8);
         break;
      }

      glformat = tex_conv_table[res->base.format].glformat;
      gltype = tex_conv_table[res->base.format].gltype;

      if ((!vrend_state.use_core_profile) && (res->y_0_top)) {
         GLuint buffers;
         GLuint fb_id;

         glGenFramebuffers(1, &fb_id);
         glBindFramebuffer(GL_FRAMEBUFFER, fb_id);
         vrend_fb_bind_texture(res, 0, info->level, 0);

         buffers = GL_COLOR_ATTACHMENT0;
         glDrawBuffers(1, &buffers);
         glDisable(GL_BLEND);
         if (ctx) {
            vrend_depth_test_enable(ctx, false);
            vrend_alpha_test_enable(ctx, false);
            vrend_stencil_test_enable(ctx->sub, false);
         } else {
            glDisable(GL_DEPTH_TEST);
            glDisable(GL_ALPHA_TEST);
            glDisable(GL_STENCIL_TEST);
         }
         glPixelZoom(1.0f, res->y_0_top ? -1.0f : 1.0f);
         glWindowPos2i(info->box->x, res->y_0_top ? (int)res->base.height0 - info->box->y : info->box->y);
         glDrawPixels(info->box->width, info->box->height, glformat, gltype,
                      data);
         glDeleteFramebuffers(1, &fb_id);
      } else {
         uint32_t comp_size;
         GLint old_tex = 0;
         get_current_texture(res->target, &old_tex);
         glBindTexture(res->target, res->id);

         if (compressed) {
            glformat = tex_conv_table[res->base.format].internalformat;
            comp_size = util_format_get_nblocks(res->base.format, info->box->width,
                                                info->box->height) * util_format_get_blocksize(res->base.format);
         }

         if (glformat == 0) {
            glformat = GL_BGRA;
            gltype = GL_UNSIGNED_BYTE;
         }

         x = info->box->x;
         y = invert ? (int)res->base.height0 - info->box->y - info->box->height : info->box->y;

         /* GLES doesn't allow format conversions, which we need for BGRA resources with RGBA
          * internal format. So we fallback to performing a CPU swizzle before uploading. */
         if (vrend_state.use_gles && vrend_format_is_bgra(res->base.format) &&
             !vrend_resource_is_emulated_bgra(res)) {
            VREND_DEBUG(dbg_bgra, ctx, "manually swizzling bgra->rgba on upload since gles+bgra\n");
            vrend_swizzle_data_bgra(send_size, data);
         }

         /* mipmaps are usually passed in one iov, and we need to keep the offset
          * into the data in case we want to read back the data of a surface
          * that can not be rendered. Since we can not assume that the whole texture
          * is filled, we evaluate the offset for origin (0,0,0). Since it is also
          * possible that a resource is reused and resized update the offset every time.
          */
         if (info->level < VR_MAX_TEXTURE_2D_LEVELS) {
            int64_t level_height = u_minify(res->base.height0, info->level);
            res->mipmap_offsets[info->level] = info->offset -
                                               ((info->box->z * level_height + y) * stride + x * elsize);
         }

         if (res->base.format == VIRGL_FORMAT_Z24X8_UNORM) {
            /* we get values from the guest as 24-bit scaled integers
               but we give them to the host GL and it interprets them
               as 32-bit scaled integers, so we need to scale them here */
            depth_scale = 256.0;
            if (!vrend_state.use_core_profile)
               glPixelTransferf(GL_DEPTH_SCALE, depth_scale);
            else
               vrend_scale_depth(data, send_size, depth_scale);
         }
         if (res->target == GL_TEXTURE_CUBE_MAP) {
            GLenum ctarget = GL_TEXTURE_CUBE_MAP_POSITIVE_X + info->box->z;
            if (compressed) {
               glCompressedTexSubImage2D(ctarget, info->level, x, y,
                                         info->box->width, info->box->height,
                                         glformat, comp_size, data);
            } else {
               glTexSubImage2D(ctarget, info->level, x, y, info->box->width, info->box->height,
                               glformat, gltype, data);
            }
         } else if (res->target == GL_TEXTURE_3D || res->target == GL_TEXTURE_2D_ARRAY || res->target == GL_TEXTURE_CUBE_MAP_ARRAY) {
            if (compressed) {
               glCompressedTexSubImage3D(res->target, info->level, x, y, info->box->z,
                                         info->box->width, info->box->height, info->box->depth,
                                         glformat, comp_size, data);
            } else {
               glTexSubImage3D(res->target, info->level, x, y, info->box->z,
                               info->box->width, info->box->height, info->box->depth,
                               glformat, gltype, data);
            }
         } else if (res->target == GL_TEXTURE_1D) {
            if (vrend_state.use_gles) {
               /* Covers both compressed and none compressed. */
               report_gles_missing_func(ctx, "gl[Compressed]TexSubImage1D");
            } else if (compressed) {
               glCompressedTexSubImage1D(res->target, info->level, info->box->x,
                                         info->box->width,
                                         glformat, comp_size, data);
            } else {
               glTexSubImage1D(res->target, info->level, info->box->x, info->box->width,
                               glformat, gltype, data);
            }
         } else {
            if (compressed) {
               glCompressedTexSubImage2D(res->target, info->level, x, res->target == GL_TEXTURE_1D_ARRAY ? info->box->z : y,
                                         info->box->width, info->box->height,
                                         glformat, comp_size, data);
            } else {
               glTexSubImage2D(res->target, info->level, x, res->target == GL_TEXTURE_1D_ARRAY ? info->box->z : y,
                               info->box->width,
                               res->target == GL_TEXTURE_1D_ARRAY ? info->box->depth : info->box->height,
                               glformat, gltype, data);
            }
         }
         if (res->base.format == VIRGL_FORMAT_Z24X8_UNORM) {
            if (!vrend_state.use_core_profile)
               glPixelTransferf(GL_DEPTH_SCALE, 1.0);
         }
         glBindTexture(res->target, old_tex);
      }

      if (stride && !need_temp) {
         glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
         glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, 0);
      }

      glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

      if (need_temp)
         free(data);
   }
   return 0;
}