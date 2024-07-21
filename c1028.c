dfafree (struct dfa *d)
{
  int i;
  struct dfamust *dm, *ndm;

  free(d->charclasses);
  free(d->tokens);

  if (d->mb_cur_max > 1)
    free_mbdata(d);

  for (i = 0; i < d->sindex; ++i) {
    free(d->states[i].elems.elems);
    if (MBS_SUPPORT)
      free(d->states[i].mbps.elems);
  }
  free(d->states);
  for (i = 0; i < d->tindex; ++i)
    free(d->follows[i].elems);
  free(d->follows);
  for (i = 0; i < d->tralloc; ++i)
    {
      free(d->trans[i]);
      free(d->fails[i]);
    }
  free(d->realtrans);
  free(d->fails);
  free(d->newlines);
  free(d->success);
  for (dm = d->musts; dm; dm = ndm)
    {
      ndm = dm->next;
      free(dm->must);
      free(dm);
    }
}