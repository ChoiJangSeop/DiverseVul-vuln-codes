void CLASS lossy_dng_load_raw()
{
  struct jpeg_decompress_struct cinfo;
  struct jpeg_error_mgr jerr;
  JSAMPARRAY buf;
  JSAMPLE (*pixel)[3];
  unsigned sorder=order, ntags, opcode, deg, i, j, c;
  unsigned save=data_offset-4, trow=0, tcol=0, row, col;
  ushort cur[3][256];
  double coeff[9], tot;

  if (meta_offset) {
    fseek (ifp, meta_offset, SEEK_SET);
    order = 0x4d4d;
    ntags = get4();
    while (ntags--) {
      opcode = get4(); get4(); get4();
      if (opcode != 8)
      { fseek (ifp, get4(), SEEK_CUR); continue; }
      fseek (ifp, 20, SEEK_CUR);
      if ((c = get4()) > 2) break;
      fseek (ifp, 12, SEEK_CUR);
      if ((deg = get4()) > 8) break;
      for (i=0; i <= deg && i < 9; i++)
	coeff[i] = getreal(12);
      for (i=0; i < 256; i++) {
	for (tot=j=0; j <= deg; j++)
	  tot += coeff[j] * pow(i/255.0, (int)j);
	cur[c][i] = tot*0xffff;
      }
    }
    order = sorder;
  } else {
    gamma_curve (1/2.4, 12.92, 1, 255);
    FORC3 memcpy (cur[c], curve, sizeof cur[0]);
  }
  cinfo.err = jpeg_std_error (&jerr);
  jpeg_create_decompress (&cinfo);
  while (trow < raw_height) {
    fseek (ifp, save+=4, SEEK_SET);
    if (tile_length < INT_MAX)
      fseek (ifp, get4(), SEEK_SET);
#ifdef LIBRAW_LIBRARY_BUILD
    if(libraw_internal_data.internal_data.input->jpeg_src(&cinfo) == -1)
      {
        jpeg_destroy_decompress(&cinfo);
        throw LIBRAW_EXCEPTION_DECODE_JPEG;
      }
#else
    jpeg_stdio_src (&cinfo, ifp);
#endif
    jpeg_read_header (&cinfo, TRUE);
    jpeg_start_decompress (&cinfo);
    buf = (*cinfo.mem->alloc_sarray)
	((j_common_ptr) &cinfo, JPOOL_IMAGE, cinfo.output_width*3, 1);
#ifdef LIBRAW_LIBRARY_BUILD
  try {
#endif
    while (cinfo.output_scanline < cinfo.output_height &&
	(row = trow + cinfo.output_scanline) < height) {
#ifdef LIBRAW_LIBRARY_BUILD
    checkCancel();
#endif
      jpeg_read_scanlines (&cinfo, buf, 1);
      pixel = (JSAMPLE (*)[3]) buf[0];
      for (col=0; col < cinfo.output_width && tcol+col < width; col++) {
	FORC3 image[row*width+tcol+col][c] = cur[c][pixel[col][c]];
      }
    }
#ifdef LIBRAW_LIBRARY_BUILD
  } catch(...) {
    jpeg_destroy_decompress (&cinfo);
    throw;
  }
#endif
    jpeg_abort_decompress (&cinfo);
    if ((tcol += tile_width) >= raw_width)
      trow += tile_length + (tcol = 0);
  }
  jpeg_destroy_decompress (&cinfo);
  maximum = 0xffff;
}