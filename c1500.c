void CLASS lossless_jpeg_load_raw()
{
  int jwide, jrow, jcol, val, c, i, row=0, col=0;
#ifndef LIBRAW_LIBRARY_BUILD
  int jidx,j;
#endif
  struct jhead jh;
  int min=INT_MAX;
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
      val = *rp++;
      if (jh.bits <= 12)
	val = curve[val & 0xfff];
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
#ifndef LIBRAW_LIBRARY_BUILD

      if (raw_width == 3984 && (col -= 2) < 0)
              col += (row--,raw_width);

      if ((unsigned) (row-top_margin) < height) {
	c = FC(row-top_margin,col-left_margin);
	if ((unsigned) (col-left_margin) < width) {
	  BAYER(row-top_margin,col-left_margin) = val;
	  if (min > val) min = val;
	} else if (col > 1 && (unsigned) (col-left_margin+2) > width+3)
	  cblack[c] += (cblack[4+c]++,val);
      }
#else
      if (raw_width == 3984)
          {
              if ( (col -= 2) < 0)
                  col += (row--,raw_width);
              if(row >= 0 && row < raw_height && col >= 0 && col < raw_width)
                  RBAYER(row,col) = val;
          }
      else 
          RBAYER(row,col) = val;

      if ((unsigned) (row-top_margin) < height) 
          {
              // within image height
              if ((unsigned) (col-left_margin) < width) 
                  {
                      // within image area, save min
                      if(save_min)
                          if (min > val) min = val;
                  } 
              else if (col > 1 && (unsigned) (col-left_margin+2) > width+3) 
                  {
                      c = FC(row-top_margin,col-left_margin);
                      cblack[c] += (cblack[4+c]++,val);
                  }
          }
#endif

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
  FORC4 if (cblack[4+c]) cblack[c] /= cblack[4+c];
  if (!strcasecmp(make,"KODAK"))
    black = min;
#ifdef LIBRAW_LIBRARY_BUILD
  if(buf)
      delete buf;
  free(offset);
#endif
}