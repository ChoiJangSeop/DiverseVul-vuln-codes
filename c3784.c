pixops_composite_color_nearest (guchar        *dest_buf,
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
				int            overall_alpha,
				int            check_x,
				int            check_y,
				int            check_size,
				guint32        color1,
				guint32        color2)
{
  int i, j;
  int x;
  int x_step = (1 << SCALE_SHIFT) / scale_x;
  int y_step = (1 << SCALE_SHIFT) / scale_y;
  int r1, g1, b1, r2, g2, b2;
  int check_shift = get_check_shift (check_size);
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
      
      
      if (((i + check_y) >> check_shift) & 1)
	{
	  r1 = (color2 & 0xff0000) >> 16;
	  g1 = (color2 & 0xff00) >> 8;
	  b1 = color2 & 0xff;

	  r2 = (color1 & 0xff0000) >> 16;
	  g2 = (color1 & 0xff00) >> 8;
	  b2 = color1 & 0xff;
	}
      else
	{
	  r1 = (color1 & 0xff0000) >> 16;
	  g1 = (color1 & 0xff00) >> 8;
	  b1 = color1 & 0xff;

	  r2 = (color2 & 0xff0000) >> 16;
	  g2 = (color2 & 0xff00) >> 8;
	  b2 = color2 & 0xff;
	}

      j = 0;
      INNER_LOOP(src_channels, dest_channels,
	  if (src_has_alpha)
	    a0 = (p[3] * overall_alpha + 0xff) >> 8;
	  else
	    a0 = overall_alpha;

          switch (a0)
            {
            case 0:
              if (((j + check_x) >> check_shift) & 1)
                {
                  dest[0] = r2; 
                  dest[1] = g2; 
                  dest[2] = b2;
                }
              else
                {
                  dest[0] = r1; 
                  dest[1] = g1; 
                  dest[2] = b1;
                }
            break;
            case 255:
	      dest[0] = p[0];
	      dest[1] = p[1];
	      dest[2] = p[2];
              break;
            default:
		     {
		       unsigned int tmp;
              if (((j + check_x) >> check_shift) & 1)
                {
                  tmp = ((int) p[0] - r2) * a0;
                  dest[0] = r2 + ((tmp + (tmp >> 8) + 0x80) >> 8);
                  tmp = ((int) p[1] - g2) * a0;
                  dest[1] = g2 + ((tmp + (tmp >> 8) + 0x80) >> 8);
                  tmp = ((int) p[2] - b2) * a0;
                  dest[2] = b2 + ((tmp + (tmp >> 8) + 0x80) >> 8);
                }
              else
                {
                  tmp = ((int) p[0] - r1) * a0;
                  dest[0] = r1 + ((tmp + (tmp >> 8) + 0x80) >> 8);
                  tmp = ((int) p[1] - g1) * a0;
                  dest[1] = g1 + ((tmp + (tmp >> 8) + 0x80) >> 8);
                  tmp = ((int) p[2] - b1) * a0;
                  dest[2] = b1 + ((tmp + (tmp >> 8) + 0x80) >> 8);
                }
		     }
              break;
            }
	  
	  if (dest_channels == 4)
	    dest[3] = 0xff;

		 j++;
	);
    }
}