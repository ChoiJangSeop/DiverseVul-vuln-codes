jinit_merged_upsampler(j_decompress_ptr cinfo)
{
  my_upsample_ptr upsample;

  upsample = (my_upsample_ptr)
    (*cinfo->mem->alloc_small) ((j_common_ptr)cinfo, JPOOL_IMAGE,
                                sizeof(my_upsampler));
  cinfo->upsample = (struct jpeg_upsampler *)upsample;
  upsample->pub.start_pass = start_pass_merged_upsample;
  upsample->pub.need_context_rows = FALSE;

  upsample->out_row_width = cinfo->output_width * cinfo->out_color_components;

  if (cinfo->max_v_samp_factor == 2) {
    upsample->pub.upsample = merged_2v_upsample;
    if (jsimd_can_h2v2_merged_upsample())
      upsample->upmethod = jsimd_h2v2_merged_upsample;
    else
      upsample->upmethod = h2v2_merged_upsample;
    if (cinfo->out_color_space == JCS_RGB565) {
      if (cinfo->dither_mode != JDITHER_NONE) {
        upsample->upmethod = h2v2_merged_upsample_565D;
      } else {
        upsample->upmethod = h2v2_merged_upsample_565;
      }
    }
    /* Allocate a spare row buffer */
    upsample->spare_row = (JSAMPROW)
      (*cinfo->mem->alloc_large) ((j_common_ptr)cinfo, JPOOL_IMAGE,
                (size_t)(upsample->out_row_width * sizeof(JSAMPLE)));
  } else {
    upsample->pub.upsample = merged_1v_upsample;
    if (jsimd_can_h2v1_merged_upsample())
      upsample->upmethod = jsimd_h2v1_merged_upsample;
    else
      upsample->upmethod = h2v1_merged_upsample;
    if (cinfo->out_color_space == JCS_RGB565) {
      if (cinfo->dither_mode != JDITHER_NONE) {
        upsample->upmethod = h2v1_merged_upsample_565D;
      } else {
        upsample->upmethod = h2v1_merged_upsample_565;
      }
    }
    /* No spare row needed */
    upsample->spare_row = NULL;
  }

  build_ycc_rgb_table(cinfo);
}