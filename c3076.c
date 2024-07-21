make_weights (PixopsFilter     *filter,
	      PixopsInterpType  interp_type,	      
	      double            scale_x,
	      double            scale_y)
{
  switch (interp_type)
    {
    case PIXOPS_INTERP_NEAREST:
      g_assert_not_reached ();
      break;

    case PIXOPS_INTERP_TILES:
      tile_make_weights (&filter->x, scale_x);
      tile_make_weights (&filter->y, scale_y);
      break;
      
    case PIXOPS_INTERP_BILINEAR:
      bilinear_magnify_make_weights (&filter->x, scale_x);
      bilinear_magnify_make_weights (&filter->y, scale_y);
      break;
      
    case PIXOPS_INTERP_HYPER:
      bilinear_box_make_weights (&filter->x, scale_x);
      bilinear_box_make_weights (&filter->y, scale_y);
      break;
    }
}