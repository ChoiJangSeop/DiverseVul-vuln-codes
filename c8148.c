static void vrend_renderer_blit_int(struct vrend_context *ctx,
                                    struct vrend_resource *src_res,
                                    struct vrend_resource *dst_res,
                                    const struct pipe_blit_info *info)
{
   GLbitfield glmask = 0;
   int src_y1, src_y2, dst_y1, dst_y2;
   GLenum filter;
   int n_layers = 1, i;
   bool use_gl = false;
   bool make_intermediate_copy = false;
   bool skip_dest_swizzle = false;
   GLuint intermediate_fbo = 0;
   struct vrend_resource *intermediate_copy = 0;

   GLuint blitter_views[2] = {src_res->id, dst_res->id};

   filter = convert_mag_filter(info->filter);

   /* if we can't make FBO's use the fallback path */
   if (!vrend_format_can_render(src_res->base.format) &&
       !vrend_format_is_ds(src_res->base.format))
      use_gl = true;
   if (!vrend_format_can_render(dst_res->base.format) &&
       !vrend_format_is_ds(dst_res->base.format))
      use_gl = true;

   /* different depth formats */
   if (vrend_format_is_ds(src_res->base.format) &&
       vrend_format_is_ds(dst_res->base.format)) {
      if (src_res->base.format != dst_res->base.format) {
         if (!(src_res->base.format == PIPE_FORMAT_S8_UINT_Z24_UNORM &&
               (dst_res->base.format == PIPE_FORMAT_Z24X8_UNORM))) {
            use_gl = true;
         }
      }
   }
   /* glBlitFramebuffer - can support depth stencil with NEAREST
      which we use for mipmaps */
   if ((info->mask & (PIPE_MASK_Z | PIPE_MASK_S)) && info->filter == PIPE_TEX_FILTER_LINEAR)
      use_gl = true;

   /* for scaled MS blits we either need extensions or hand roll */
   if (info->mask & PIPE_MASK_RGBA &&
       src_res->base.nr_samples > 0 &&
       src_res->base.nr_samples != dst_res->base.nr_samples &&
       (info->src.box.width != info->dst.box.width ||
        info->src.box.height != info->dst.box.height)) {
      if (has_feature(feat_ms_scaled_blit))
         filter = GL_SCALED_RESOLVE_NICEST_EXT;
      else
         use_gl = true;
   }

   if (!dst_res->y_0_top) {
      dst_y1 = info->dst.box.y + info->dst.box.height;
      dst_y2 = info->dst.box.y;
   } else {
      dst_y1 = dst_res->base.height0 - info->dst.box.y - info->dst.box.height;
      dst_y2 = dst_res->base.height0 - info->dst.box.y;
   }

   if (!src_res->y_0_top) {
      src_y1 = info->src.box.y + info->src.box.height;
      src_y2 = info->src.box.y;
   } else {
      src_y1 = src_res->base.height0 - info->src.box.y - info->src.box.height;
      src_y2 = src_res->base.height0 - info->src.box.y;
   }

   /* GLES generally doesn't support blitting to a multi-sample FB, and also not
    * from a multi-sample FB where the regions are not exatly the same or the
    * source and target format are different. For
    * downsampling DS blits to zero samples we solve this by doing two blits */
   if (vrend_state.use_gles &&
       ((dst_res->base.nr_samples > 0) ||
        ((info->mask & PIPE_MASK_RGBA) &&
         (src_res->base.nr_samples > 0) &&
         (info->src.box.x != info->dst.box.x ||
          info->src.box.width != info->dst.box.width ||
          dst_y1 != src_y1 || dst_y2 != src_y2 ||
          info->src.format != info->dst.format))
        )
       ) {
      VREND_DEBUG(dbg_blit, ctx, "Use GL fallback because dst:ms:%d src:ms:%d (%d %d %d %d) -> (%d %d %d %d)\n",
                  dst_res->base.nr_samples, src_res->base.nr_samples, info->src.box.x, info->src.box.x + info->src.box.width,
                  src_y1, src_y2, info->dst.box.x, info->dst.box.x + info->dst.box.width, dst_y1, dst_y2);
      use_gl = true;
   }

   /* for 3D mipmapped blits - hand roll time */
   if (info->src.box.depth != info->dst.box.depth)
      use_gl = true;

   if (vrend_blit_needs_swizzle(vrend_format_replace_emulated(dst_res->base.bind, info->dst.format),
                                vrend_format_replace_emulated(src_res->base.bind, info->src.format))) {
      use_gl = true;

      if (vrend_state.use_gles &&
          (dst_res->base.bind & VIRGL_BIND_PREFER_EMULATED_BGRA) &&
          !vrend_get_tweak_is_active(&ctx->sub->tweaks, virgl_tweak_gles_brga_apply_dest_swizzle)) {
         skip_dest_swizzle = true;
      }
   }

   if (has_feature(feat_texture_view))
      blitter_views[0] = vrend_make_view(src_res, info->src.format);

   if ((dst_res->base.format != info->dst.format) && has_feature(feat_texture_view))
      blitter_views[1] = vrend_make_view(dst_res, info->dst.format);


   if (use_gl) {
      VREND_DEBUG(dbg_blit, ctx, "BLIT_INT: use GL fallback\n");
      vrend_renderer_blit_gl(ctx, src_res, dst_res, blitter_views, info,
                             has_feature(feat_texture_srgb_decode),
                             has_feature(feat_srgb_write_control),
                             skip_dest_swizzle);
      vrend_clicbs->make_current(ctx->sub->gl_context);
      goto cleanup;
   }

   if (info->mask & PIPE_MASK_Z)
      glmask |= GL_DEPTH_BUFFER_BIT;
   if (info->mask & PIPE_MASK_S)
      glmask |= GL_STENCIL_BUFFER_BIT;
   if (info->mask & PIPE_MASK_RGBA)
      glmask |= GL_COLOR_BUFFER_BIT;


   if (info->scissor_enable) {
      glScissor(info->scissor.minx, info->scissor.miny, info->scissor.maxx - info->scissor.minx, info->scissor.maxy - info->scissor.miny);
      ctx->sub->scissor_state_dirty = (1 << 0);
      glEnable(GL_SCISSOR_TEST);
   } else
      glDisable(GL_SCISSOR_TEST);

   /* An GLES GL_INVALID_OPERATION is generated if one wants to blit from a
    * multi-sample fbo to a non multi-sample fbo and the source and destination
    * rectangles are not defined with the same (X0, Y0) and (X1, Y1) bounds.
    *
    * Since stencil data can only be written in a fragment shader when
    * ARB_shader_stencil_export is available, the workaround using GL as given
    * above is usually not available. Instead, to work around the blit
    * limitations on GLES first copy the full frame to a non-multisample
    * surface and then copy the according area to the final target surface.
    */
   if (vrend_state.use_gles &&
       (info->mask & PIPE_MASK_ZS) &&
       ((src_res->base.nr_samples > 0) &&
        (src_res->base.nr_samples != dst_res->base.nr_samples)) &&
        ((info->src.box.x != info->dst.box.x) ||
         (src_y1 != dst_y1) ||
         (info->src.box.width != info->dst.box.width) ||
         (src_y2 != dst_y2))) {

      make_intermediate_copy = true;

      /* Create a texture that is the same like the src_res texture, but
       * without multi-sample */
      struct vrend_renderer_resource_create_args args;
      memset(&args, 0, sizeof(struct vrend_renderer_resource_create_args));
      args.width = src_res->base.width0;
      args.height = src_res->base.height0;
      args.depth = src_res->base.depth0;
      args.format = info->src.format;
      args.target = src_res->base.target;
      args.last_level = src_res->base.last_level;
      args.array_size = src_res->base.array_size;
      intermediate_copy = (struct vrend_resource *)CALLOC_STRUCT(vrend_texture);
      vrend_renderer_resource_copy_args(&args, intermediate_copy);
      vrend_renderer_resource_allocate_texture(intermediate_copy, NULL);

      glGenFramebuffers(1, &intermediate_fbo);
   } else {
      /* If no intermediate copy is needed make the variables point to the
       * original source to simplify the code below.
       */
      intermediate_fbo = ctx->sub->blit_fb_ids[0];
      intermediate_copy = src_res;
   }

   glBindFramebuffer(GL_FRAMEBUFFER, ctx->sub->blit_fb_ids[0]);
   if (info->mask & PIPE_MASK_RGBA)
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                             GL_TEXTURE_2D, 0, 0);
   else
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                             GL_TEXTURE_2D, 0, 0);
   glBindFramebuffer(GL_FRAMEBUFFER, ctx->sub->blit_fb_ids[1]);
   if (info->mask & PIPE_MASK_RGBA)
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                             GL_TEXTURE_2D, 0, 0);
   else if (info->mask & (PIPE_MASK_Z | PIPE_MASK_S))
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                             GL_TEXTURE_2D, 0, 0);
   if (info->src.box.depth == info->dst.box.depth)
      n_layers = info->dst.box.depth;
   for (i = 0; i < n_layers; i++) {
      glBindFramebuffer(GL_FRAMEBUFFER, ctx->sub->blit_fb_ids[0]);
      vrend_fb_bind_texture_id(src_res, blitter_views[0], 0, info->src.level, info->src.box.z + i);

      if (make_intermediate_copy) {
         int level_width = u_minify(src_res->base.width0, info->src.level);
         int level_height = u_minify(src_res->base.width0, info->src.level);
         glBindFramebuffer(GL_FRAMEBUFFER, intermediate_fbo);
         glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                GL_TEXTURE_2D, 0, 0);
         vrend_fb_bind_texture(intermediate_copy, 0, info->src.level, info->src.box.z + i);

         glBindFramebuffer(GL_DRAW_FRAMEBUFFER, intermediate_fbo);
         glBindFramebuffer(GL_READ_FRAMEBUFFER, ctx->sub->blit_fb_ids[0]);
         glBlitFramebuffer(0, 0, level_width, level_height,
                           0, 0, level_width, level_height,
                           glmask, filter);
      }

      glBindFramebuffer(GL_FRAMEBUFFER, ctx->sub->blit_fb_ids[1]);
      vrend_fb_bind_texture_id(dst_res, blitter_views[1], 0, info->dst.level, info->dst.box.z + i);
      glBindFramebuffer(GL_DRAW_FRAMEBUFFER, ctx->sub->blit_fb_ids[1]);

      if (has_feature(feat_srgb_write_control)) {
         if (util_format_is_srgb(info->dst.format) ||
             util_format_is_srgb(info->src.format))
            glEnable(GL_FRAMEBUFFER_SRGB);
         else
            glDisable(GL_FRAMEBUFFER_SRGB);
      }

      glBindFramebuffer(GL_READ_FRAMEBUFFER, intermediate_fbo);

      glBlitFramebuffer(info->src.box.x,
                        src_y1,
                        info->src.box.x + info->src.box.width,
                        src_y2,
                        info->dst.box.x,
                        dst_y1,
                        info->dst.box.x + info->dst.box.width,
                        dst_y2,
                        glmask, filter);
   }

   glBindFramebuffer(GL_FRAMEBUFFER, ctx->sub->blit_fb_ids[1]);
   glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                          GL_TEXTURE_2D, 0, 0);
   glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                          GL_TEXTURE_2D, 0, 0);

   glBindFramebuffer(GL_FRAMEBUFFER, ctx->sub->blit_fb_ids[0]);
   glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                          GL_TEXTURE_2D, 0, 0);
   glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                          GL_TEXTURE_2D, 0, 0);

   glBindFramebuffer(GL_FRAMEBUFFER, ctx->sub->fb_id);

   if (has_feature(feat_srgb_write_control)) {
      if (ctx->sub->framebuffer_srgb_enabled)
         glEnable(GL_FRAMEBUFFER_SRGB);
      else
         glDisable(GL_FRAMEBUFFER_SRGB);
   }

   if (make_intermediate_copy) {
      vrend_renderer_resource_destroy(intermediate_copy);
      glDeleteFramebuffers(1, &intermediate_fbo);
   }

   if (ctx->sub->rs_state.scissor)
      glEnable(GL_SCISSOR_TEST);
   else
      glDisable(GL_SCISSOR_TEST);

cleanup:
   if (blitter_views[0] != src_res->id)
      glDeleteTextures(1, &blitter_views[0]);

   if (blitter_views[1] != dst_res->id)
      glDeleteTextures(1, &blitter_views[1]);
}