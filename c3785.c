pixops_composite_nearest (guchar        *dest_buf,
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
			  double         scale_y,
			  int            overall_alpha)
{
  int i;
  int x;
  int x_step = (1 << SCALE_SHIFT) / scale_x;
  int y_step = (1 << SCALE_SHIFT) / scale_y;
  int xmax, xstart, xstop, x_pos, y_pos;
  const guchar *p;
  unsigned int  a0;

  for (i = 0; i < (render_y1 - render_y0); i++)
    {
      const guchar *src;
      guchar       *dest;
      y_pos = ((i + render_y0) * y_step + y_step / 2) >> SCALE_SHIFT;
      y_pos = CLAMP (y_pos, 0, src_height - 1);
      src  = src_buf + (gsize)y_pos * src_rowstride;
      dest = dest_buf + (gsize)i * dest_rowstride;

      x = render_x0 * x_step + x_step / 2;
      
      INNER_LOOP(src_channels, dest_channels,
	  if (src_has_alpha)
	    a0 = (p[3] * overall_alpha) / 0xff;
	  else
	    a0 = overall_alpha;

          switch (a0)
            {
            case 0:
              break;
            case 255:
              dest[0] = p[0];
              dest[1] = p[1];
              dest[2] = p[2];
              if (dest_has_alpha)
                dest[3] = 0xff;
              break;
            default:
              if (dest_has_alpha)
                {
                  unsigned int w0 = 0xff * a0;
                  unsigned int w1 = (0xff - a0) * dest[3];
                  unsigned int w = w0 + w1;

		  dest[0] = (w0 * p[0] + w1 * dest[0]) / w;
		  dest[1] = (w0 * p[1] + w1 * dest[1]) / w;
		  dest[2] = (w0 * p[2] + w1 * dest[2]) / w;
		  dest[3] = w / 0xff;
                }
              else
                {
                  unsigned int a1 = 0xff - a0;
		  unsigned int tmp;

		  tmp = a0 * p[0] + a1 * dest[0] + 0x80;
                  dest[0] = (tmp + (tmp >> 8)) >> 8;
		  tmp = a0 * p[1] + a1 * dest[1] + 0x80;
                  dest[1] = (tmp + (tmp >> 8)) >> 8;
		  tmp = a0 * p[2] + a1 * dest[2] + 0x80;
                  dest[2] = (tmp + (tmp >> 8)) >> 8;
                }
              break;
            }
	);
    }
}