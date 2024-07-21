void CLASS kodak_rgb_load_raw()
{
  short buf[768], *bp;
  int row, col, len, c, i, rgb[3],ret;
  ushort *ip=image[0];

  for (row=0; row < height; row++)
  {
#ifdef LIBRAW_LIBRARY_BUILD
    checkCancel();
#endif
    for (col=0; col < width; col+=256) {
      len = MIN (256, width-col);
      ret = kodak_65000_decode (buf, len*3);
      memset (rgb, 0, sizeof rgb);
      for (bp=buf, i=0; i < len; i++, ip+=4)
#ifdef LIBRAW_LIBRARY_BUILD
        if(load_flags == 12)
          {
            FORC3 ip[c] = ret ? (*bp++) : (rgb[c] += *bp++);
          }
        else
#endif
          FORC3 if ((ip[c] = ret ? (*bp++) : (rgb[c] += *bp++)) >> 12) derror();
    }
  }
}