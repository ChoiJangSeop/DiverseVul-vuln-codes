transit_state (struct dfa *d, int s, unsigned char const **pp)
{
  int s1;
  int mbclen;		/* The length of current input multibyte character. */
  int maxlen = 0;
  int i, j;
  int *match_lens = NULL;
  int nelem = d->states[s].mbps.nelem; /* Just a alias.  */
  position_set follows;
  unsigned char const *p1 = *pp;
  wchar_t wc;

  if (nelem > 0)
    /* This state has (a) multibyte operator(s).
       We check whether each of them can match or not.  */
    {
      /* Note: caller must free the return value of this function.  */
      match_lens = check_matching_with_multibyte_ops(d, s, *pp - buf_begin);

      for (i = 0; i < nelem; i++)
        /* Search the operator which match the longest string,
           in this state.  */
        {
          if (match_lens[i] > maxlen)
            maxlen = match_lens[i];
        }
    }

  if (nelem == 0 || maxlen == 0)
    /* This state has no multibyte operator which can match.
       We need to check only one single byte character.  */
    {
      status_transit_state rs;
      rs = transit_state_singlebyte(d, s, *pp, &s1);

      /* We must update the pointer if state transition succeeded.  */
      if (rs == TRANSIT_STATE_DONE)
        ++*pp;

      free(match_lens);
      return s1;
    }

  /* This state has some operators which can match a multibyte character.  */
  alloc_position_set(&follows, d->nleaves);

  /* `maxlen' may be longer than the length of a character, because it may
     not be a character but a (multi character) collating element.
     We enumerate all of the positions which `s' can reach by consuming
     `maxlen' bytes.  */
  transit_state_consume_1char(d, s, pp, match_lens, &mbclen, &follows);

  wc = inputwcs[*pp - mbclen - buf_begin];
  s1 = state_index(d, &follows, wchar_context (wc));
  realloc_trans_if_necessary(d, s1);

  while (*pp - p1 < maxlen)
    {
      transit_state_consume_1char(d, s1, pp, NULL, &mbclen, &follows);

      for (i = 0; i < nelem ; i++)
        {
          if (match_lens[i] == *pp - p1)
            for (j = 0;
                 j < d->follows[d->states[s1].mbps.elems[i].index].nelem; j++)
              insert(d->follows[d->states[s1].mbps.elems[i].index].elems[j],
                     &follows);
        }

      wc = inputwcs[*pp - mbclen - buf_begin];
      s1 = state_index(d, &follows, wchar_context (wc));
      realloc_trans_if_necessary(d, s1);
    }
  free(match_lens);
  free(follows.elems);
  return s1;
}