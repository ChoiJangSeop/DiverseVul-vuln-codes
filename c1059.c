grepbuf (char const *beg, char const *lim)
{
  int nlines, n;
  char const *p;
  size_t match_offset;
  size_t match_size;

  nlines = 0;
  p = beg;
  while ((match_offset = do_execute (p, lim - p, &match_size,
                                     NULL)) != (size_t) -1)
    {
      char const *b = p + match_offset;
      char const *endp = b + match_size;
      /* Avoid matching the empty line at the end of the buffer. */
      if (b == lim)
        break;
      if (!out_invert)
        {
          prtext (b, endp, (int *) 0);
          nlines++;
          outleft--;
          if (!outleft || done_on_match)
            {
              if (exit_on_match)
                exit (EXIT_SUCCESS);
              after_last_match = bufoffset - (buflim - endp);
              return nlines;
            }
        }
      else if (p < b)
        {
          prtext (p, b, &n);
          nlines += n;
          outleft -= n;
          if (!outleft)
            return nlines;
        }
      p = endp;
    }
  if (out_invert && p < lim)
    {
      prtext (p, lim, &n);
      nlines += n;
      outleft -= n;
    }
  return nlines;
}