_pixops_composite_color_real (guchar          *dest_buf,
			      int              render_x0,
			      int              render_y0,
			      int              render_x1,
			      int              render_y1,
			      int              dest_rowstride,
			      int              dest_channels,
			      gboolean         dest_has_alpha,
			      const guchar    *src_buf,
			      int              src_width,
			      int              src_height,
			      int              src_rowstride,
			      int              src_channels,
			      gboolean         src_has_alpha,
			      double           scale_x,
			      double           scale_y,
			      PixopsInterpType interp_type,
			      int              overall_alpha,
			      int              check_x,
			      int              check_y,
			      int              check_size,
			      guint32          color1,
			      guint32          color2)
{
  PixopsFilter filter;
  PixopsLineFunc line_func;
  
#ifdef USE_MMX
  gboolean found_mmx = _pixops_have_mmx ();
#endif

  g_return_if_fail (!(dest_channels == 3 && dest_has_alpha));
  g_return_if_fail (!(src_channels == 3 && src_has_alpha));

  if (scale_x == 0 || scale_y == 0)
    return;

  if (interp_type == PIXOPS_INTERP_NEAREST)
    {
      pixops_composite_color_nearest (dest_buf, render_x0, render_y0,
				      render_x1, render_y1, dest_rowstride,
				      dest_channels, dest_has_alpha, src_buf,
				      src_width, src_height, src_rowstride,
				      src_channels, src_has_alpha, scale_x,
				      scale_y, overall_alpha, check_x, check_y,
				      check_size, color1, color2);
      return;
    }
  
  filter.overall_alpha = overall_alpha / 255.;
  make_weights (&filter, interp_type, scale_x, scale_y);

#ifdef USE_MMX
  if (filter.x.n == 2 && filter.y.n == 2 &&
      dest_channels == 4 && src_channels == 4 &&
      src_has_alpha && !dest_has_alpha && found_mmx)
    line_func = composite_line_color_22_4a4_mmx_stub;
  else
#endif
    line_func = composite_line_color;
  
  pixops_process (dest_buf, render_x0, render_y0, render_x1, render_y1,
		  dest_rowstride, dest_channels, dest_has_alpha,
		  src_buf, src_width, src_height, src_rowstride, src_channels,
		  src_has_alpha, scale_x, scale_y, check_x, check_y, check_size, color1, color2,
		  &filter, line_func, composite_pixel_color);

  g_free (filter.x.weights);
  g_free (filter.y.weights);
}