_pixops_scale_real (guchar        *dest_buf,
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
		    PixopsInterpType  interp_type)
{
  PixopsFilter filter;
  PixopsLineFunc line_func;

#ifdef USE_MMX
  gboolean found_mmx = _pixops_have_mmx ();
#endif

  g_return_if_fail (!(dest_channels == 3 && dest_has_alpha));
  g_return_if_fail (!(src_channels == 3 && src_has_alpha));
  g_return_if_fail (!(src_has_alpha && !dest_has_alpha));

  if (scale_x == 0 || scale_y == 0)
    return;

  if (interp_type == PIXOPS_INTERP_NEAREST)
    {
      pixops_scale_nearest (dest_buf, render_x0, render_y0, render_x1,
			    render_y1, dest_rowstride, dest_channels,
			    dest_has_alpha, src_buf, src_width, src_height,
			    src_rowstride, src_channels, src_has_alpha,
			    scale_x, scale_y);
      return;
    }
  
  filter.overall_alpha = 1.0;
  make_weights (&filter, interp_type, scale_x, scale_y);

  if (filter.x.n == 2 && filter.y.n == 2 && dest_channels == 3 && src_channels == 3)
    {
#ifdef USE_MMX
      if (found_mmx)
	line_func = scale_line_22_33_mmx_stub;
      else
#endif
	line_func = scale_line_22_33;
    }
  else
    line_func = scale_line;
  
  pixops_process (dest_buf, render_x0, render_y0, render_x1, render_y1,
		  dest_rowstride, dest_channels, dest_has_alpha,
		  src_buf, src_width, src_height, src_rowstride, src_channels,
		  src_has_alpha, scale_x, scale_y, 0, 0, 0, 0, 0,
		  &filter, line_func, scale_pixel);

  g_free (filter.x.weights);
  g_free (filter.y.weights);
}