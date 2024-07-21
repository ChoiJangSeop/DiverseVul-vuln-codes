void CLASS lossless_jpeg_load_raw()
{
  int jwide, jrow, jcol, val, i, row=0, col=0;
#ifndef LIBRAW_LIBRARY_BUILD
  int jidx,j;
#endif
  struct jhead jh;
  ushort *rp;

#ifdef LIBRAW_LIBRARY_BUILD
  int save_min = 0;
  unsigned slicesW[16],slicesWcnt=0,slices;
  unsigned *offset;
  unsigned t_y=0,t_x=0,t_s=0,slice=0,pixelsInSlice,pixno;
  if (!strcasecmp(make,"KODAK"))
      save_min = 1;
#endif

#ifdef LIBRAW_LIBRARY_BUILD
  if (cr2_slice[0]>15)
      throw LIBRAW_EXCEPTION_IO_EOF; // change many slices
#else
  if (cr2_slice[0]>15)
  {
      fprintf(stderr,"Too many CR2 slices: %d\n",cr2_slice[0]+1);
      return;
  }
#endif


  if (!ljpeg_start (&jh, 0)) return;
  jwide = jh.wide * jh.clrs;

#ifdef LIBRAW_LIBRARY_BUILD
  if(cr2_slice[0])
      {
          for(i=0;i<cr2_slice[0];i++)
              slicesW[slicesWcnt++] = cr2_slice[1];
          slicesW[slicesWcnt++] = cr2_slice[2];
      }
  else
      {
          // not sliced
          slicesW[slicesWcnt++] = raw_width; // safe fallback
      }
       
  slices = slicesWcnt * jh.high;
  offset = (unsigned*)calloc(slices+1,sizeof(offset[0]));

  for(slice=0;slice<slices;slice++)
      {
          offset[slice] = (t_x + t_y * raw_width)| (t_s<<28);
          if((offset[slice] & 0x0fffffff) >= raw_width * raw_height)
              throw LIBRAW_EXCEPTION_IO_BADFILE; 
          t_y++;
          if(t_y == jh.high)
              {
                  t_y = 0;
                  t_x += slicesW[t_s++];
              }
      }
  offset[slices] = offset[slices-1];
  slice = 1; // next slice
  pixno = offset[0]; 
  pixelsInSlice = slicesW[0];
#endif

#ifdef LIBRAW_LIBRARY_BUILD
  LibRaw_byte_buffer *buf=NULL;
  if(data_size)
      buf = ifp->make_byte_buffer(data_size);
  LibRaw_bit_buffer bits;
#endif

  for (jrow=0; jrow < jh.high; jrow++) {
#ifdef LIBRAW_LIBRARY_BUILD
      if (buf)
          rp = ljpeg_row_new (jrow, &jh,bits,buf);
      else
#endif
    rp = ljpeg_row (jrow, &jh);
    if (load_flags & 1)
      row = jrow & 1 ? height-1-jrow/2 : jrow/2;
    for (jcol=0; jcol < jwide; jcol++) {
      val = curve[*rp++];
#ifndef LIBRAW_LIBRARY_BUILD
      // slow dcraw way to calculate row/col
      if (cr2_slice[0]) {
	jidx = jrow*jwide + jcol;
	i = jidx / (cr2_slice[1]*jh.high);
	if ((j = i >= cr2_slice[0]))
		 i  = cr2_slice[0];
	jidx -= i * (cr2_slice[1]*jh.high);
	row = jidx / cr2_slice[1+j];
	col = jidx % cr2_slice[1+j] + i*cr2_slice[1];
      }
#else
      // new fast one, but for data_size defined only (i.e. new CR2 format, not 1D/1Ds)
      if(buf) 
          {
              if(!(load_flags & 1))
                  row = pixno/raw_width;
              col = pixno % raw_width;
              pixno++;
              if (0 == --pixelsInSlice)
                  {
                      unsigned o = offset[slice++];
                      pixno = o & 0x0fffffff;
                      pixelsInSlice = slicesW[o>>28];
                  }
          }
#endif
      if (raw_width == 3984 && (col -= 2) < 0)
	col += (row--,raw_width);
      if (row >= 0) RAW(row,col) = val;
#ifndef LIBRAW_LIBRARY_BUILD
      if (++col >= raw_width)
	col = (row++,0);
#else
      if(!buf) // 1D or 1Ds case
         if (++col >= raw_width)
            col = (row++,0);
#endif
    }
  }
  ljpeg_end (&jh);
#ifdef LIBRAW_LIBRARY_BUILD
  if(buf)
      delete buf;
  free(offset);
#endif
}