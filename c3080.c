bilinear_magnify_make_weights (PixopsFilterDimension *dim,
			       double                 scale)
{
  double *pixel_weights;
  int n;
  int offset;
  int i;

  if (scale > 1.0)            /* Linear */
    {
      n = 2;
      dim->offset = 0.5 * (1 / scale - 1);
    }
  else                          /* Tile */
    {
      n = ceil (1.0 + 1.0 / scale);
      dim->offset = 0.0;
    }

  dim->n = n;
  dim->weights = g_malloc_n (sizeof (double) * SUBSAMPLE, n);

  pixel_weights = dim->weights;

  for (offset=0; offset < SUBSAMPLE; offset++)
    {
      double x = (double)offset / SUBSAMPLE;

      if (scale > 1.0)      /* Linear */
        {
          for (i = 0; i < n; i++)
            *(pixel_weights++) = (((i == 0) ? (1 - x) : x) / scale) * scale;
        }
      else                  /* Tile */
        {
          double a = x + 1 / scale;

          /*           x
           * ---------|--.-|----|--.-|-------  SRC
           * ------------|---------|---------  DEST
           */
          for (i = 0; i < n; i++)
            {
              if (i < x)
                {
                  if (i + 1 > x)
                    *(pixel_weights++) = (MIN (i + 1, a) - x) * scale;
                  else
                    *(pixel_weights++) = 0;
                }
              else
                {
                  if (a > i)
                    *(pixel_weights++) = (MIN (i + 1, a) - i) * scale;
                  else
                    *(pixel_weights++) = 0;
                }
            }
        }
    }
}