state_index (struct dfa *d, position_set const *s, int context)
{
  int hash = 0;
  int constraint;
  int i, j;

  for (i = 0; i < s->nelem; ++i)
    hash ^= s->elems[i].index + s->elems[i].constraint;

  /* Try to find a state that exactly matches the proposed one. */
  for (i = 0; i < d->sindex; ++i)
    {
      if (hash != d->states[i].hash || s->nelem != d->states[i].elems.nelem
          || context != d->states[i].context)
        continue;
      for (j = 0; j < s->nelem; ++j)
        if (s->elems[j].constraint
            != d->states[i].elems.elems[j].constraint
            || s->elems[j].index != d->states[i].elems.elems[j].index)
          break;
      if (j == s->nelem)
        return i;
    }

  /* We'll have to create a new state. */
  REALLOC_IF_NECESSARY(d->states, d->salloc, d->sindex + 1);
  d->states[i].hash = hash;
  alloc_position_set(&d->states[i].elems, s->nelem);
  copy(s, &d->states[i].elems);
  d->states[i].context = context;
  d->states[i].backref = 0;
  d->states[i].constraint = 0;
  d->states[i].first_end = 0;
  if (MBS_SUPPORT)
    {
      d->states[i].mbps.nelem = 0;
      d->states[i].mbps.elems = NULL;
    }
  for (j = 0; j < s->nelem; ++j)
    if (d->tokens[s->elems[j].index] < 0)
      {
        constraint = s->elems[j].constraint;
        if (SUCCEEDS_IN_CONTEXT (constraint, context, CTX_ANY))
          d->states[i].constraint |= constraint;
        if (! d->states[i].first_end)
          d->states[i].first_end = d->tokens[s->elems[j].index];
      }
    else if (d->tokens[s->elems[j].index] == BACKREF)
      {
        d->states[i].constraint = NO_CONSTRAINT;
        d->states[i].backref = 1;
      }

  ++d->sindex;

  return i;
}