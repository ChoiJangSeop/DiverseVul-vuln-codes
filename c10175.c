read_and_discard_scanlines(j_decompress_ptr cinfo, JDIMENSION num_lines)
{
  JDIMENSION n;
  my_master_ptr master = (my_master_ptr)cinfo->master;
  JSAMPARRAY scanlines = NULL;
  void (*color_convert) (j_decompress_ptr cinfo, JSAMPIMAGE input_buf,
                         JDIMENSION input_row, JSAMPARRAY output_buf,
                         int num_rows) = NULL;
  void (*color_quantize) (j_decompress_ptr cinfo, JSAMPARRAY input_buf,
                          JSAMPARRAY output_buf, int num_rows) = NULL;

  if (cinfo->cconvert && cinfo->cconvert->color_convert) {
    color_convert = cinfo->cconvert->color_convert;
    cinfo->cconvert->color_convert = noop_convert;
  }

  if (cinfo->cquantize && cinfo->cquantize->color_quantize) {
    color_quantize = cinfo->cquantize->color_quantize;
    cinfo->cquantize->color_quantize = noop_quantize;
  }

  if (master->using_merged_upsample && cinfo->max_v_samp_factor == 2) {
    my_merged_upsample_ptr upsample = (my_merged_upsample_ptr)cinfo->upsample;
    scanlines = &upsample->spare_row;
  }

  for (n = 0; n < num_lines; n++)
    jpeg_read_scanlines(cinfo, scanlines, 1);

  if (color_convert)
    cinfo->cconvert->color_convert = color_convert;

  if (color_quantize)
    cinfo->cquantize->color_quantize = color_quantize;
}