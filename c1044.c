match_anychar (struct dfa *d, int s, position pos, int idx)
{
  int context;
  wchar_t wc;
  int mbclen;

  wc = inputwcs[idx];
  mbclen = (mblen_buf[idx] == 0)? 1 : mblen_buf[idx];

  /* Check syntax bits.  */
  if (wc == (wchar_t)eolbyte)
    {
      if (!(syntax_bits & RE_DOT_NEWLINE))
        return 0;
    }
  else if (wc == (wchar_t)'\0')
    {
      if (syntax_bits & RE_DOT_NOT_NULL)
        return 0;
    }

  context = wchar_context(wc);
  if (!SUCCEEDS_IN_CONTEXT(pos.constraint, d->states[s].context, context))
    return 0;

  return mbclen;
}