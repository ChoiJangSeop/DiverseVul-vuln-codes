prtext (char const *beg, char const *lim, int *nlinesp)
{
  static int used;	/* avoid printing SEP_STR_GROUP before any output */
  char const *bp, *p;
  char eol = eolbyte;
  int i, n;

  if (!out_quiet && pending > 0)
    prpending (beg);

  p = beg;

  if (!out_quiet)
    {
      /* Deal with leading context crap. */

      bp = lastout ? lastout : bufbeg;
      for (i = 0; i < out_before; ++i)
        if (p > bp)
          do
            --p;
          while (p[-1] != eol);

      /* We print the SEP_STR_GROUP separator only if our output is
         discontiguous from the last output in the file. */
      if ((out_before || out_after) && used && p != lastout && group_separator)
        {
          pr_sgr_start_if (sep_color);
          fputs (group_separator, stdout);
          pr_sgr_end_if (sep_color);
          fputc ('\n', stdout);
        }

      while (p < beg)
        {
          char const *nl = memchr (p, eol, beg - p);
          nl++;
          prline (p, nl, SEP_CHAR_REJECTED);
          p = nl;
        }
    }

  if (nlinesp)
    {
      /* Caller wants a line count. */
      for (n = 0; p < lim && n < outleft; n++)
        {
          char const *nl = memchr (p, eol, lim - p);
          nl++;
          if (!out_quiet)
            prline (p, nl, SEP_CHAR_SELECTED);
          p = nl;
        }
      *nlinesp = n;

      /* relying on it that this function is never called when outleft = 0.  */
      after_last_match = bufoffset - (buflim - p);
    }
  else if (!out_quiet)
    prline (beg, lim, SEP_CHAR_SELECTED);

  pending = out_quiet ? 0 : out_after;
  used = 1;
}