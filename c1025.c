realloc_trans_if_necessary(struct dfa *d, int new_state)
{
  /* Make sure that the trans and fail arrays are allocated large enough
     to hold a pointer for the new state. */
  if (new_state >= d->tralloc)
    {
      int oldalloc = d->tralloc;

      while (new_state >= d->tralloc)
        d->tralloc *= 2;
      REALLOC(d->realtrans, d->tralloc + 1);
      d->trans = d->realtrans + 1;
      REALLOC(d->fails, d->tralloc);
      REALLOC(d->success, d->tralloc);
      REALLOC(d->newlines, d->tralloc);
      while (oldalloc < d->tralloc)
        {
          d->trans[oldalloc] = NULL;
          d->fails[oldalloc++] = NULL;
        }
    }
}