make_filter_table (PixopsFilter *filter)
{
  int i_offset, j_offset;
  int n_x = filter->x.n;
  int n_y = filter->y.n;
  int *weights = g_new (int, SUBSAMPLE * SUBSAMPLE * n_x * n_y);

  for (i_offset=0; i_offset < SUBSAMPLE; i_offset++)
    for (j_offset=0; j_offset < SUBSAMPLE; j_offset++)
      {
        double weight;
        int *pixel_weights = weights + ((i_offset*SUBSAMPLE) + j_offset) * n_x * n_y;
        int total = 0;
        int i, j;

        for (i=0; i < n_y; i++)
          for (j=0; j < n_x; j++)
            {
              weight = filter->x.weights[(j_offset * n_x) + j] *
                       filter->y.weights[(i_offset * n_y) + i] *
                       filter->overall_alpha * 65536 + 0.5;

              total += (int)weight;

              *(pixel_weights + n_x * i + j) = weight;
            }

        correct_total (pixel_weights, n_x, n_y, total, filter->overall_alpha);
      }

  return weights;
}