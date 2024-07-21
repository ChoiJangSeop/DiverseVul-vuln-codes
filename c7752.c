new_aubio_filterbank (uint_t n_filters, uint_t win_s)
{
  /* allocate space for filterbank object */
  aubio_filterbank_t *fb = AUBIO_NEW (aubio_filterbank_t);
  fb->win_s = win_s;
  fb->n_filters = n_filters;

  /* allocate filter tables, a matrix of length win_s and of height n_filters */
  fb->filters = new_fmat (n_filters, win_s / 2 + 1);

  fb->norm = 1;

  fb->power = 1;

  return fb;
}