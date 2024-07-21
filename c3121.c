pixops_scale_nearest (guchar        *dest_buf,
		      int            render_x0,
		      int            render_y0,
		      int            render_x1,
		      int            render_y1,
		      int            dest_rowstride,
		      int            dest_channels,
		      gboolean       dest_has_alpha,
		      const guchar  *src_buf,
		      int            src_width,
		      int            src_height,
		      int            src_rowstride,
		      int            src_channels,
		      gboolean       src_has_alpha,
		      double         scale_x,
		      double         scale_y)
{
  int i;
  int x;
  int x_step = (1 << SCALE_SHIFT) / scale_x;
  int y_step = (1 << SCALE_SHIFT) / scale_y;
  int xmax, xstart, xstop, x_pos, y_pos;
  const guchar *p;

#define INNER_LOOP(SRC_CHANNELS,DEST_CHANNELS,ASSIGN_PIXEL)     \
      xmax = x + (render_x1 - render_x0) * x_step;              \
      xstart = MIN (0, xmax);                                   \
      xstop = MIN (src_width << SCALE_SHIFT, xmax);             \
      p = src + (CLAMP (x, xstart, xstop) >> SCALE_SHIFT) * SRC_CHANNELS; \
      while (x < xstart)                                        \
        {                                                       \
          ASSIGN_PIXEL;                                         \
          dest += DEST_CHANNELS;                                \
          x += x_step;                                          \
        }                                                       \
      while (x < xstop)                                         \
        {                                                       \
          p = src + (x >> SCALE_SHIFT) * SRC_CHANNELS;          \
          ASSIGN_PIXEL;                                         \
          dest += DEST_CHANNELS;                                \
          x += x_step;                                          \
        }                                                       \
      x_pos = x >> SCALE_SHIFT;                                 \
      p = src + CLAMP (x_pos, 0, src_width - 1) * SRC_CHANNELS; \
      while (x < xmax)                                          \
        {                                                       \
          ASSIGN_PIXEL;                                         \
          dest += DEST_CHANNELS;                                \
          x += x_step;                                          \
        }

  for (i = 0; i < (render_y1 - render_y0); i++)
    {
      const guchar *src;
      guchar       *dest;
      y_pos = ((i + render_y0) * y_step + y_step / 2) >> SCALE_SHIFT;
      y_pos = CLAMP (y_pos, 0, src_height - 1);
      src  = src_buf + (gsize)y_pos * src_rowstride;
      dest = dest_buf + (gsize)i * dest_rowstride;

      x = render_x0 * x_step + x_step / 2;

      if (src_channels == 3)
	{
	  if (dest_channels == 3)
	    {
	      INNER_LOOP (3, 3, dest[0]=p[0];dest[1]=p[1];dest[2]=p[2]);
	    }
	  else
	    {
	      INNER_LOOP (3, 4, dest[0]=p[0];dest[1]=p[1];dest[2]=p[2];dest[3]=0xff);
	    }
	}
      else if (src_channels == 4)
	{
	  if (dest_channels == 3)
	    {
	      INNER_LOOP (4, 3, dest[0]=p[0];dest[1]=p[1];dest[2]=p[2]);
	    }
	  else
	    {
	      guint32 *p32;
	      INNER_LOOP(4, 4, p32=(guint32*)dest;*p32=*((guint32*)p));
	    }
	}
    }
}