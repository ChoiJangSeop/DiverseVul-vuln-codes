free_mbdata (struct dfa *d)
{
  unsigned int i;

  free(d->multibyte_prop);
  d->multibyte_prop = NULL;

  for (i = 0; i < d->nmbcsets; ++i)
    {
      unsigned int j;
      struct mb_char_classes *p = &(d->mbcsets[i]);
      free(p->chars);
      free(p->ch_classes);
      free(p->range_sts);
      free(p->range_ends);

      for (j = 0; j < p->nequivs; ++j)
        free(p->equivs[j]);
      free(p->equivs);

      for (j = 0; j < p->ncoll_elems; ++j)
        free(p->coll_elems[j]);
      free(p->coll_elems);
    }

  free(d->mbcsets);
  d->mbcsets = NULL;
  d->nmbcsets = 0;
}