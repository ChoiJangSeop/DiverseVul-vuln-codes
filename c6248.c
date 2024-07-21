get_text_gray_row (j_compress_ptr cinfo, cjpeg_source_ptr sinfo)
/* This version is for reading text-format PGM files with any maxval */
{
  ppm_source_ptr source = (ppm_source_ptr) sinfo;
  FILE * infile = source->pub.input_file;
  register JSAMPROW ptr;
  register JSAMPLE *rescale = source->rescale;
  JDIMENSION col;

  ptr = source->pub.buffer[0];
  for (col = cinfo->image_width; col > 0; col--) {
    *ptr++ = rescale[read_pbm_integer(cinfo, infile)];
  }
  return 1;
}