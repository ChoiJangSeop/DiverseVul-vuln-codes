EGexecute (char const *buf, size_t size, size_t *match_size,
           char const *start_ptr)
{
  char const *buflim, *beg, *end, *match, *best_match, *mb_start;
  char eol = eolbyte;
  int backref, start, len, best_len;
  struct kwsmatch kwsm;
  size_t i, ret_val;
  if (MB_CUR_MAX > 1)
    {
      if (match_icase)
        {
          /* mbtolower adds a NUL byte at the end.  That will provide
             space for the sentinel byte dfaexec may add.  */
          char *case_buf = mbtolower (buf, &size);
          if (start_ptr)
            start_ptr = case_buf + (start_ptr - buf);
          buf = case_buf;
        }
    }

  mb_start = buf;
  buflim = buf + size;

  for (beg = end = buf; end < buflim; beg = end)
    {
      if (!start_ptr)
        {
          /* We don't care about an exact match.  */
          if (kwset)
            {
              /* Find a possible match using the KWset matcher. */
              size_t offset = kwsexec (kwset, beg, buflim - beg, &kwsm);
              if (offset == (size_t) -1)
                goto failure;
              beg += offset;
              /* Narrow down to the line containing the candidate, and
                 run it through DFA. */
              if ((end = memchr(beg, eol, buflim - beg)) != NULL)
                end++;
              else
                end = buflim;
              match = beg;
              while (beg > buf && beg[-1] != eol)
                --beg;
              if (kwsm.index < kwset_exact_matches)
                {
                  if (!MBS_SUPPORT)
                    goto success;

                  if (mb_start < beg)
                    mb_start = beg;
                  if (MB_CUR_MAX == 1
                      || !is_mb_middle (&mb_start, match, buflim,
                                        kwsm.size[0]))
                    goto success;
                }
              if (dfaexec (dfa, beg, (char *) end, 0, NULL, &backref) == NULL)
                continue;
            }
          else
            {
              /* No good fixed strings; start with DFA. */
              char const *next_beg = dfaexec (dfa, beg, (char *) buflim,
                                              0, NULL, &backref);
              if (next_beg == NULL)
                break;
              /* Narrow down to the line we've found. */
              beg = next_beg;
              if ((end = memchr(beg, eol, buflim - beg)) != NULL)
                end++;
              else
                end = buflim;
              while (beg > buf && beg[-1] != eol)
                --beg;
            }
          /* Successful, no backreferences encountered! */
          if (!backref)
            goto success;
        }
      else
        {
          /* We are looking for the leftmost (then longest) exact match.
             We will go through the outer loop only once.  */
          beg = start_ptr;
          end = buflim;
        }

      /* If we've made it to this point, this means DFA has seen
         a probable match, and we need to run it through Regex. */
      best_match = end;
      best_len = 0;
      for (i = 0; i < pcount; i++)
        {
          patterns[i].regexbuf.not_eol = 0;
          if (0 <= (start = re_search (&(patterns[i].regexbuf),
                                       buf, end - buf - 1,
                                       beg - buf, end - beg - 1,
                                       &(patterns[i].regs))))
            {
              len = patterns[i].regs.end[0] - start;
              match = buf + start;
              if (match > best_match)
                continue;
              if (start_ptr && !match_words)
                goto assess_pattern_match;
              if ((!match_lines && !match_words)
                  || (match_lines && len == end - beg - 1))
                {
                  match = beg;
                  len = end - beg;
                  goto assess_pattern_match;
                }
              /* If -w, check if the match aligns with word boundaries.
                 We do this iteratively because:
                 (a) the line may contain more than one occurence of the
                 pattern, and
                 (b) Several alternatives in the pattern might be valid at a
                 given point, and we may need to consider a shorter one to
                 find a word boundary.  */
              if (match_words)
                while (match <= best_match)
                  {
                    if ((match == buf || !WCHAR ((unsigned char) match[-1]))
                        && (start + len == end - buf - 1
                            || !WCHAR ((unsigned char) match[len])))
                      goto assess_pattern_match;
                    if (len > 0)
                      {
                        /* Try a shorter length anchored at the same place. */
                        --len;
                        patterns[i].regexbuf.not_eol = 1;
                        len = re_match (&(patterns[i].regexbuf),
                                        buf, match + len - beg, match - buf,
                                        &(patterns[i].regs));
                      }
                    if (len <= 0)
                      {
                        /* Try looking further on. */
                        if (match == end - 1)
                          break;
                        match++;
                        patterns[i].regexbuf.not_eol = 0;
                        start = re_search (&(patterns[i].regexbuf),
                                           buf, end - buf - 1,
                                           match - buf, end - match - 1,
                                           &(patterns[i].regs));
                        if (start < 0)
                          break;
                        len = patterns[i].regs.end[0] - start;
                        match = buf + start;
                      }
                  } /* while (match <= best_match) */
              continue;
            assess_pattern_match:
              if (!start_ptr)
                {
                  /* Good enough for a non-exact match.
                     No need to look at further patterns, if any.  */
                  goto success;
                }
              if (match < best_match || (match == best_match && len > best_len))
                {
                  /* Best exact match:  leftmost, then longest.  */
                  best_match = match;
                  best_len = len;
                }
            } /* if re_search >= 0 */
        } /* for Regex patterns.  */
        if (best_match < end)
          {
            /* We have found an exact match.  We were just
               waiting for the best one (leftmost then longest).  */
            beg = best_match;
            len = best_len;
            goto success_in_len;
          }
    } /* for (beg = end ..) */

 failure:
  ret_val = -1;
  goto out;

 success:
  len = end - beg;
 success_in_len:
  *match_size = len;
  ret_val = beg - buf;
 out:
  return ret_val;
}