int vrend_renderer_resource_create(struct vrend_renderer_resource_create_args *args, struct iovec *iov, uint32_t num_iovs, void *image_oes)
{
   struct vrend_resource *gr;
   int ret;

   ret = check_resource_valid(args);
   if (ret)
      return EINVAL;

   gr = (struct vrend_resource *)CALLOC_STRUCT(vrend_texture);
   if (!gr)
      return ENOMEM;

   vrend_renderer_resource_copy_args(args, gr);
   gr->iov = iov;
   gr->num_iovs = num_iovs;

   if (args->flags & VIRGL_RESOURCE_Y_0_TOP)
      gr->y_0_top = true;

   pipe_reference_init(&gr->base.reference, 1);

   if (args->bind == VIRGL_BIND_CUSTOM) {
      assert(args->target == PIPE_BUFFER);
      /* use iovec directly when attached */
      gr->storage = VREND_RESOURCE_STORAGE_GUEST_ELSE_SYSTEM;
      gr->ptr = malloc(args->width);
      if (!gr->ptr) {
         FREE(gr);
         return ENOMEM;
      }
   } else if (args->bind == VIRGL_BIND_STAGING) {
      /* Staging buffers use only guest memory. */
      gr->storage = VREND_RESOURCE_STORAGE_GUEST;
   } else if (args->bind == VIRGL_BIND_INDEX_BUFFER) {
      gr->target = GL_ELEMENT_ARRAY_BUFFER_ARB;
      vrend_create_buffer(gr, args->width);
   } else if (args->bind == VIRGL_BIND_STREAM_OUTPUT) {
      gr->target = GL_TRANSFORM_FEEDBACK_BUFFER;
      vrend_create_buffer(gr, args->width);
   } else if (args->bind == VIRGL_BIND_VERTEX_BUFFER) {
      gr->target = GL_ARRAY_BUFFER_ARB;
      vrend_create_buffer(gr, args->width);
   } else if (args->bind == VIRGL_BIND_CONSTANT_BUFFER) {
      gr->target = GL_UNIFORM_BUFFER;
      vrend_create_buffer(gr, args->width);
   } else if (args->bind == VIRGL_BIND_QUERY_BUFFER) {
      gr->target = GL_QUERY_BUFFER;
      vrend_create_buffer(gr, args->width);
   } else if (args->bind == VIRGL_BIND_COMMAND_ARGS) {
      gr->target = GL_DRAW_INDIRECT_BUFFER;
      vrend_create_buffer(gr, args->width);
   } else if (args->target == PIPE_BUFFER && (args->bind == 0 || args->bind == VIRGL_BIND_SHADER_BUFFER)) {
      gr->target = GL_ARRAY_BUFFER_ARB;
      vrend_create_buffer(gr, args->width);
   } else if (args->target == PIPE_BUFFER && (args->bind & VIRGL_BIND_SAMPLER_VIEW)) {
      /*
       * On Desktop we use GL_ARB_texture_buffer_object on GLES we use
       * GL_EXT_texture_buffer (it is in the ANDRIOD extension pack).
       */
#if GL_TEXTURE_BUFFER != GL_TEXTURE_BUFFER_EXT
#error "GL_TEXTURE_BUFFER enums differ, they shouldn't."
#endif

      /* need to check GL version here */
      if (has_feature(feat_arb_or_gles_ext_texture_buffer)) {
         gr->target = GL_TEXTURE_BUFFER;
      } else {
         gr->target = GL_PIXEL_PACK_BUFFER_ARB;
      }
      vrend_create_buffer(gr, args->width);
   } else {
      int r = vrend_renderer_resource_allocate_texture(gr, image_oes);
      if (r)
         return r;
   }

   ret = vrend_resource_insert(gr, args->handle);
   if (ret == 0) {
      vrend_renderer_resource_destroy(gr);
      return ENOMEM;
   }
   return 0;
}