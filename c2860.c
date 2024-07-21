bilinear_box_make_weights (PixopsFilterDimension *dim,
			   double                 scale)
{
  int n = ceil (1/scale + 3.0);
  double *pixel_weights = g_new (double, SUBSAMPLE * n);
  double w;
  int offset, i;

  dim->offset = -1.0;
  dim->n = n;
  dim->weights = pixel_weights;

  for (offset = 0; offset < SUBSAMPLE; offset++)
    {
      double x = (double)offset / SUBSAMPLE;
      double a = x + 1 / scale;

      for (i = 0; i < n; i++)
        {
          w  = linear_box_half (0.5 + i - a, 0.5 + i - x);
          w += linear_box_half (1.5 + x - i, 1.5 + a - i);
      
          *(pixel_weights++) = w * scale;
        }
    }
}