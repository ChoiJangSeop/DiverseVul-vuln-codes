read_and_discard_scanlines(j_decompress_ptr cinfo, JDIMENSION num_lines)
{
  JDIMENSION n;
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

  for (n = 0; n < num_lines; n++)
    jpeg_read_scanlines(cinfo, NULL, 1);

  if (color_convert)
    cinfo->cconvert->color_convert = color_convert;

  if (color_quantize)
    cinfo->cquantize->color_quantize = color_quantize;
}