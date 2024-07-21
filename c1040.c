build_state (int s, struct dfa *d)
{
  int *trans;			/* The new transition table. */
  int i;

  /* Set an upper limit on the number of transition tables that will ever
     exist at once.  1024 is arbitrary.  The idea is that the frequently
     used transition tables will be quickly rebuilt, whereas the ones that
     were only needed once or twice will be cleared away. */
  if (d->trcount >= 1024)
    {
      for (i = 0; i < d->tralloc; ++i)
        {
          free(d->trans[i]);
          free(d->fails[i]);
          d->trans[i] = d->fails[i] = NULL;
        }
      d->trcount = 0;
    }

  ++d->trcount;

  /* Set up the success bits for this state. */
  d->success[s] = 0;
  if (ACCEPTS_IN_CONTEXT(d->states[s].context, CTX_NEWLINE, s, *d))
    d->success[s] |= CTX_NEWLINE;
  if (ACCEPTS_IN_CONTEXT(d->states[s].context, CTX_LETTER, s, *d))
    d->success[s] |= CTX_LETTER;
  if (ACCEPTS_IN_CONTEXT(d->states[s].context, CTX_NONE, s, *d))
    d->success[s] |= CTX_NONE;

  MALLOC(trans, NOTCHAR);
  dfastate(s, d, trans);

  /* Now go through the new transition table, and make sure that the trans
     and fail arrays are allocated large enough to hold a pointer for the
     largest state mentioned in the table. */
  for (i = 0; i < NOTCHAR; ++i)
    if (trans[i] >= d->tralloc)
      {
        int oldalloc = d->tralloc;

        while (trans[i] >= d->tralloc)
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

  /* Keep the newline transition in a special place so we can use it as
     a sentinel. */
  d->newlines[s] = trans[eolbyte];
  trans[eolbyte] = -1;

  if (ACCEPTING(s, *d))
    d->fails[s] = trans;
  else
    d->trans[s] = trans;
}