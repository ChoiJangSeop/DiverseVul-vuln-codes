void CLASS kodak_c603_load_raw()
{
  uchar *pixel;
  int row, col, y, cb, cr, rgb[3], c;

  pixel = (uchar *) calloc (raw_width, 3*sizeof *pixel);
  merror (pixel, "kodak_c603_load_raw()");
#ifdef LIBRAW_LIBRARY_BUILD
  try {
#endif
  for (row=0; row < height; row++) {
#ifdef LIBRAW_LIBRARY_BUILD
    checkCancel();
#endif
    if (~row & 1)
      if (fread (pixel, raw_width, 3, ifp) < 3) derror();
    for (col=0; col < width; col++) {
      y  = pixel[width*2*(row & 1) + col];
      cb = pixel[width + (col & -2)]   - 128;
      cr = pixel[width + (col & -2)+1] - 128;
      rgb[1] = y - ((cb + cr + 2) >> 2);
      rgb[2] = rgb[1] + cb;
      rgb[0] = rgb[1] + cr;
      FORC3 image[row*width+col][c] = curve[LIM(rgb[c],0,255)];
    }
  }
#ifdef LIBRAW_LIBRARY_BUILD
  } catch(...) {
    free (pixel);
    throw;
  }
#endif
  free (pixel);
  maximum = curve[0xff];
}