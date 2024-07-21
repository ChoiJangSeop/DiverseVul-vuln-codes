void CLASS nikon_yuv_load_raw()
{
  int row, col, yuv[4], rgb[3], b, c;
  UINT64 bitbuf=0;
  float cmul[4];
  FORC4 { cmul[c] = cam_mul[c]>0.001f?cam_mul[c]:1.f; }
  for (row=0; row < raw_height; row++)
  {
#ifdef LIBRAW_LIBRARY_BUILD
    checkCancel();
#endif

    for (col=0; col < raw_width; col++) {
      if (!(b = col & 1)) {
	bitbuf = 0;
	FORC(6) bitbuf |= (UINT64) fgetc(ifp) << c*8;
	FORC(4) yuv[c] = (bitbuf >> c*12 & 0xfff) - (c >> 1 << 11);
      }
      rgb[0] = yuv[b] + 1.370705*yuv[3];
      rgb[1] = yuv[b] - 0.337633*yuv[2] - 0.698001*yuv[3];
      rgb[2] = yuv[b] + 1.732446*yuv[2];
      FORC3 image[row*width+col][c] = curve[LIM(rgb[c],0,0xfff)] / cmul[c];
    }
  }
}