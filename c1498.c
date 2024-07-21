void CLASS lossless_jpeg_load_raw()
{
  int jwide, jrow, jcol, val, jidx, i, j, row=0, col=0;
  struct jhead jh;
  ushort *rp;

  if (!ljpeg_start (&jh, 0)) return;
  jwide = jh.wide * jh.clrs;

#ifdef LIBRAW_LIBRARY_BUILD
  try {
#endif
  for (jrow=0; jrow < jh.high; jrow++) {
#ifdef LIBRAW_LIBRARY_BUILD
    checkCancel();
#endif
    rp = ljpeg_row (jrow, &jh);
    if (load_flags & 1)
      row = jrow & 1 ? height-1-jrow/2 : jrow/2;
    for (jcol=0; jcol < jwide; jcol++) {
      val = curve[*rp++];
      if (cr2_slice[0]) {
	jidx = jrow*jwide + jcol;
	i = jidx / (cr2_slice[1]*jh.high);
	if ((j = i >= cr2_slice[0]))
		 i  = cr2_slice[0];
	jidx -= i * (cr2_slice[1]*jh.high);
	row = jidx / cr2_slice[1+j];
	col = jidx % cr2_slice[1+j] + i*cr2_slice[1];
      }
      if (raw_width == 3984 && (col -= 2) < 0)
	col += (row--,raw_width);
      if ((unsigned) row < raw_height) RAW(row,col) = val;
      if (++col >= raw_width)
	col = (row++,0);
    }
  }
#ifdef LIBRAW_LIBRARY_BUILD
  } catch (...) {
    ljpeg_end (&jh);
    throw;
  }
#endif
  ljpeg_end (&jh);
}