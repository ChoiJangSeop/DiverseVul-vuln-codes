pixops_process (guchar         *dest_buf,
		int             render_x0,
		int             render_y0,
		int             render_x1,
		int             render_y1,
		int             dest_rowstride,
		int             dest_channels,
		gboolean        dest_has_alpha,
		const guchar   *src_buf,
		int             src_width,
		int             src_height,
		int             src_rowstride,
		int             src_channels,
		gboolean        src_has_alpha,
		double          scale_x,
		double          scale_y,
		int             check_x,
		int             check_y,
		int             check_size,
		guint32         color1,
		guint32         color2,
		PixopsFilter   *filter,
		PixopsLineFunc  line_func,
		PixopsPixelFunc pixel_func)
{
  int i, j;
  int x, y;			/* X and Y position in source (fixed_point) */

  guchar **line_bufs;
  int *filter_weights;

  int x_step;
  int y_step;

  int check_shift;
  int scaled_x_offset;

  int run_end_x;
  int run_end_index;

  x_step = (1 << SCALE_SHIFT) / scale_x; /* X step in source (fixed point) */
  y_step = (1 << SCALE_SHIFT) / scale_y; /* Y step in source (fixed point) */

  if (x_step == 0 || y_step == 0)
    return; /* overflow, bail out */

  line_bufs = g_new (guchar *, filter->y.n);
  filter_weights = make_filter_table (filter);

  check_shift = check_size ? get_check_shift (check_size) : 0;

  scaled_x_offset = floor (filter->x.offset * (1 << SCALE_SHIFT));

  /* Compute the index where we run off the end of the source buffer. The
   * furthest source pixel we access at index i is:
   *
   *  ((render_x0 + i) * x_step + scaled_x_offset) >> SCALE_SHIFT + filter->x.n - 1
   *
   * So, run_end_index is the smallest i for which this pixel is src_width,
   * i.e, for which:
   *
   *  (i + render_x0) * x_step >= ((src_width - filter->x.n + 1) << SCALE_SHIFT) - scaled_x_offset
   *
   */
#define MYDIV(a,b) ((a) > 0 ? (a) / (b) : ((a) - (b) + 1) / (b))    /* Division so that -1/5 = -1 */

  run_end_x = (((src_width - filter->x.n + 1) << SCALE_SHIFT) - scaled_x_offset);
  run_end_index = MYDIV (run_end_x + x_step - 1, x_step) - render_x0;
  run_end_index = MIN (run_end_index, render_x1 - render_x0);

  y = render_y0 * y_step + floor (filter->y.offset * (1 << SCALE_SHIFT));
  for (i = 0; i < (render_y1 - render_y0); i++)
    {
      int dest_x;
      int y_start = y >> SCALE_SHIFT;
      int x_start;
      int *run_weights = filter_weights +
                         ((y >> (SCALE_SHIFT - SUBSAMPLE_BITS)) & SUBSAMPLE_MASK) *
                         filter->x.n * filter->y.n * SUBSAMPLE;
      guchar *new_outbuf;
      guint32 tcolor1, tcolor2;

      guchar *outbuf = dest_buf + dest_rowstride * i;
      guchar *outbuf_end = outbuf + dest_channels * (render_x1 - render_x0);

      if (((i + check_y) >> check_shift) & 1)
	{
	  tcolor1 = color2;
	  tcolor2 = color1;
	}
      else
	{
	  tcolor1 = color1;
	  tcolor2 = color2;
	}

      for (j=0; j<filter->y.n; j++)
	{
	  if (y_start <  0)
	    line_bufs[j] = (guchar *)src_buf;
	  else if (y_start < src_height)
	    line_bufs[j] = (guchar *)src_buf + src_rowstride * y_start;
	  else
	    line_bufs[j] = (guchar *)src_buf + src_rowstride * (src_height - 1);

	  y_start++;
	}

      dest_x = check_x;
      x = render_x0 * x_step + scaled_x_offset;
      x_start = x >> SCALE_SHIFT;

      while (x_start < 0 && outbuf < outbuf_end)
	{
	  process_pixel (run_weights + ((x >> (SCALE_SHIFT - SUBSAMPLE_BITS)) & SUBSAMPLE_MASK) * (filter->x.n * filter->y.n), filter->x.n, filter->y.n,
			 outbuf, dest_x, dest_channels, dest_has_alpha,
			 line_bufs, src_channels, src_has_alpha,
			 x >> SCALE_SHIFT, src_width,
			 check_size, tcolor1, tcolor2, pixel_func);

	  x += x_step;
	  x_start = x >> SCALE_SHIFT;
	  dest_x++;
	  outbuf += dest_channels;
	}

      new_outbuf = (*line_func) (run_weights, filter->x.n, filter->y.n,
				 outbuf, dest_x, dest_buf + dest_rowstride *
				 i + run_end_index * dest_channels,
				 dest_channels, dest_has_alpha,
				 line_bufs, src_channels, src_has_alpha,
				 x, x_step, src_width, check_size, tcolor1,
				 tcolor2);

      dest_x += (new_outbuf - outbuf) / dest_channels;

      x = (dest_x - check_x + render_x0) * x_step + scaled_x_offset;
      outbuf = new_outbuf;

      while (outbuf < outbuf_end)
	{
	  process_pixel (run_weights + ((x >> (SCALE_SHIFT - SUBSAMPLE_BITS)) & SUBSAMPLE_MASK) * (filter->x.n * filter->y.n), filter->x.n, filter->y.n,
			 outbuf, dest_x, dest_channels, dest_has_alpha,
			 line_bufs, src_channels, src_has_alpha,
			 x >> SCALE_SHIFT, src_width,
			 check_size, tcolor1, tcolor2, pixel_func);

	  x += x_step;
	  dest_x++;
	  outbuf += dest_channels;
	}

      y += y_step;
    }

  g_free (line_bufs);
  g_free (filter_weights);
}