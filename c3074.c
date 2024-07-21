tile_make_weights (PixopsFilterDimension *dim,
		   double                 scale)
{
  int n = ceil (1 / scale + 1);
  double *pixel_weights = g_malloc_n (sizeof (double) * SUBSAMPLE, n);
  int offset;
  int i;

  dim->n = n;
  dim->offset = 0;
  dim->weights = pixel_weights;

  for (offset = 0; offset < SUBSAMPLE; offset++)
    {
      double x = (double)offset / SUBSAMPLE;
      double a = x + 1 / scale;

      for (i = 0; i < n; i++)
        {
          if (i < x)
            {
              if (i + 1 > x)
                *(pixel_weights++)  = (MIN (i + 1, a) - x) * scale;
              else
                *(pixel_weights++) = 0;
            }
          else
            {
              if (a > i)
                *(pixel_weights++)  = (MIN (i + 1, a) - i) * scale;
              else
                *(pixel_weights++) = 0;
            }
       }
    }
}