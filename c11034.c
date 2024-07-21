vrend_resource_alloc_buffer(struct vrend_resource *gr, uint32_t flags)
{
   const uint32_t bind = gr->base.bind;
   const uint32_t size = gr->base.width0;

   if (bind == VIRGL_BIND_CUSTOM) {
      /* use iovec directly when attached */
      gr->storage_bits |= VREND_STORAGE_HOST_SYSTEM_MEMORY;
      gr->ptr = malloc(size);
      if (!gr->ptr)
         return -ENOMEM;
   } else if (bind == VIRGL_BIND_STAGING) {
     /* staging buffers only use guest memory -- nothing to do. */
   } else if (bind == VIRGL_BIND_INDEX_BUFFER) {
      gr->target = GL_ELEMENT_ARRAY_BUFFER_ARB;
      vrend_create_buffer(gr, size, flags);
   } else if (bind == VIRGL_BIND_STREAM_OUTPUT) {
      gr->target = GL_TRANSFORM_FEEDBACK_BUFFER;
      vrend_create_buffer(gr, size, flags);
   } else if (bind == VIRGL_BIND_VERTEX_BUFFER) {
      gr->target = GL_ARRAY_BUFFER_ARB;
      vrend_create_buffer(gr, size, flags);
   } else if (bind == VIRGL_BIND_CONSTANT_BUFFER) {
      gr->target = GL_UNIFORM_BUFFER;
      vrend_create_buffer(gr, size, flags);
   } else if (bind == VIRGL_BIND_QUERY_BUFFER) {
      gr->target = GL_QUERY_BUFFER;
      vrend_create_buffer(gr, size, flags);
   } else if (bind == VIRGL_BIND_COMMAND_ARGS) {
      gr->target = GL_DRAW_INDIRECT_BUFFER;
      vrend_create_buffer(gr, size, flags);
   } else if (bind == 0 || bind == VIRGL_BIND_SHADER_BUFFER) {
      gr->target = GL_ARRAY_BUFFER_ARB;
      vrend_create_buffer(gr, size, flags);
   } else if (bind & VIRGL_BIND_SAMPLER_VIEW) {
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
      vrend_create_buffer(gr, size, flags);
   } else {
      vrend_printf("%s: Illegal buffer binding flags 0x%x\n", __func__, bind);
      return -EINVAL;
   }

   return 0;
}