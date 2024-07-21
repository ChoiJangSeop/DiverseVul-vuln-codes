static int check_resource_valid(struct vrend_renderer_resource_create_args *args)
{
   /* do not accept handle 0 */
   if (args->handle == 0)
      return -1;

   /* limit the target */
   if (args->target >= PIPE_MAX_TEXTURE_TYPES)
      return -1;

   if (args->format >= VIRGL_FORMAT_MAX)
      return -1;

   /* only texture 2d and 2d array can have multiple samples */
   if (args->nr_samples > 0) {
      if (!has_feature(feat_texture_multisample))
         return -1;

      if (args->target != PIPE_TEXTURE_2D && args->target != PIPE_TEXTURE_2D_ARRAY)
         return -1;
      /* multisample can't have miplevels */
      if (args->last_level > 0)
         return -1;
   }

   if (args->last_level > 0) {
      /* buffer and rect textures can't have mipmaps */
      if (args->target == PIPE_BUFFER || args->target == PIPE_TEXTURE_RECT)
         return -1;
      if (args->last_level > (floor(log2(MAX2(args->width, args->height))) + 1))
         return -1;
   }
   if (args->flags != 0 && args->flags != VIRGL_RESOURCE_Y_0_TOP)
      return -1;

   if (args->flags & VIRGL_RESOURCE_Y_0_TOP)
      if (args->target != PIPE_TEXTURE_2D && args->target != PIPE_TEXTURE_RECT)
         return -1;

   /* array size for array textures only */
   if (args->target == PIPE_TEXTURE_CUBE) {
      if (args->array_size != 6)
         return -1;
   } else if (args->target == PIPE_TEXTURE_CUBE_ARRAY) {
      if (!has_feature(feat_cube_map_array))
         return -1;
      if (args->array_size % 6)
         return -1;
   } else if (args->array_size > 1) {
      if (args->target != PIPE_TEXTURE_2D_ARRAY &&
          args->target != PIPE_TEXTURE_1D_ARRAY)
         return -1;

      if (!has_feature(feat_texture_array))
         return -1;
   }

   if (args->bind == 0 ||
       args->bind == VIRGL_BIND_CUSTOM ||
       args->bind == VIRGL_BIND_STAGING ||
       args->bind == VIRGL_BIND_INDEX_BUFFER ||
       args->bind == VIRGL_BIND_STREAM_OUTPUT ||
       args->bind == VIRGL_BIND_VERTEX_BUFFER ||
       args->bind == VIRGL_BIND_CONSTANT_BUFFER ||
       args->bind == VIRGL_BIND_QUERY_BUFFER ||
       args->bind == VIRGL_BIND_COMMAND_ARGS ||
       args->bind == VIRGL_BIND_SHADER_BUFFER) {
      if (args->target != PIPE_BUFFER)
         return -1;
      if (args->height != 1 || args->depth != 1)
         return -1;
      if (args->bind == VIRGL_BIND_QUERY_BUFFER && !has_feature(feat_qbo))
         return -1;
      if (args->bind == VIRGL_BIND_COMMAND_ARGS && !has_feature(feat_indirect_draw))
         return -1;
   } else {
      if (!((args->bind & VIRGL_BIND_SAMPLER_VIEW) ||
            (args->bind & VIRGL_BIND_DEPTH_STENCIL) ||
            (args->bind & VIRGL_BIND_RENDER_TARGET) ||
            (args->bind & VIRGL_BIND_CURSOR) ||
            (args->bind & VIRGL_BIND_SHARED) ||
            (args->bind & VIRGL_BIND_LINEAR))) {
         return -1;
      }

      if (args->target == PIPE_TEXTURE_2D ||
          args->target == PIPE_TEXTURE_RECT ||
          args->target == PIPE_TEXTURE_CUBE ||
          args->target == PIPE_TEXTURE_2D_ARRAY ||
          args->target == PIPE_TEXTURE_CUBE_ARRAY) {
         if (args->depth != 1)
            return -1;
      }
      if (args->target == PIPE_TEXTURE_1D ||
          args->target == PIPE_TEXTURE_1D_ARRAY) {
         if (args->height != 1 || args->depth != 1)
            return -1;
      }
   }
   return 0;
}