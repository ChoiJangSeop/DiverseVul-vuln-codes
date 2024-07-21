insert (position p, position_set *s)
{
  int count = s->nelem;
  int lo = 0, hi = count;
  int i;
  while (lo < hi)
    {
      int mid = ((unsigned) lo + (unsigned) hi) >> 1;
      if (s->elems[mid].index > p.index)
        lo = mid + 1;
      else
        hi = mid;
    }

  if (lo < count && p.index == s->elems[lo].index)
    {
      s->elems[lo].constraint |= p.constraint;
      return;
    }

  REALLOC_IF_NECESSARY(s->elems, s->alloc, count + 1);
  for (i = count; i > lo; i--)
    s->elems[i] = s->elems[i - 1];
  s->elems[lo] = p;
  ++s->nelem;
}