check_matching_with_multibyte_ops (struct dfa *d, int s, int idx)
{
  int i;
  int* rarray;

  MALLOC(rarray, d->states[s].mbps.nelem);
  for (i = 0; i < d->states[s].mbps.nelem; ++i)
    {
      position pos = d->states[s].mbps.elems[i];
      switch(d->tokens[pos.index])
        {
        case ANYCHAR:
          rarray[i] = match_anychar(d, s, pos, idx);
          break;
        case MBCSET:
          rarray[i] = match_mb_charset(d, s, pos, idx);
          break;
        default:
          break; /* cannot happen.  */
        }
    }
  return rarray;
}