read_non_rle_pixel(tga_source_ptr sinfo)
/* Read one Targa pixel from the input file; no RLE expansion */
{
  register FILE *infile = sinfo->pub.input_file;
  register int i;

  for (i = 0; i < sinfo->pixel_size; i++) {
    sinfo->tga_pixel[i] = (U_CHAR)getc(infile);
  }
}