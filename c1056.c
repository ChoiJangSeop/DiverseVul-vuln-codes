dfaexec (struct dfa *d, char const *begin, char *end,
         int allow_nl, int *count, int *backref)
{
  int s, s1;		/* Current state. */
  unsigned char const *p; /* Current input character. */
  int **trans, *t;	/* Copy of d->trans so it can be optimized
                                   into a register. */
  unsigned char eol = eolbyte;	/* Likewise for eolbyte.  */
  unsigned char saved_end;

  if (! d->tralloc)
    build_state_zero(d);

  s = s1 = 0;
  p = (unsigned char const *) begin;
  trans = d->trans;
  saved_end = *(unsigned char *) end;
  *end = eol;

  if (d->mb_cur_max > 1)
    {
      MALLOC(mblen_buf, end - begin + 2);
      MALLOC(inputwcs, end - begin + 2);
      memset(&mbs, 0, sizeof(mbstate_t));
      prepare_wc_buf ((const char *) p, end);
    }

  for (;;)
    {
      if (d->mb_cur_max > 1)
        while ((t = trans[s]) != NULL)
          {
            if (p > buf_end)
              break;
            s1 = s;
            SKIP_REMAINS_MB_IF_INITIAL_STATE(s, p);

            if (d->states[s].mbps.nelem == 0)
              {
                s = t[*p++];
                continue;
              }

            /* Falling back to the glibc matcher in this case gives
               better performance (up to 25% better on [a-z], for
               example) and enables support for collating symbols and
               equivalence classes.  */
            if (backref)
              {
                *backref = 1;
                free(mblen_buf);
                free(inputwcs);
                *end = saved_end;
                return (char *) p;
              }

            /* Can match with a multibyte character (and multi character
               collating element).  Transition table might be updated.  */
            s = transit_state(d, s, &p);
            trans = d->trans;
          }
      else
        {
          while ((t = trans[s]) != NULL)
            {
              s1 = t[*p++];
              if ((t = trans[s1]) == NULL)
                {
                  int tmp = s; s = s1; s1 = tmp; /* swap */
                  break;
                }
              s = t[*p++];
            }
        }

      if (s >= 0 && (char *) p <= end && d->fails[s])
        {
          if (d->success[s] & sbit[*p])
            {
              if (backref)
                *backref = (d->states[s].backref != 0);
              if (d->mb_cur_max > 1)
                {
                  free(mblen_buf);
                  free(inputwcs);
                }
              *end = saved_end;
              return (char *) p;
            }

          s1 = s;
          if (d->mb_cur_max > 1)
            {
              /* Can match with a multibyte character (and multicharacter
                 collating element).  Transition table might be updated.  */
              s = transit_state(d, s, &p);
              trans = d->trans;
            }
          else
            s = d->fails[s][*p++];
          continue;
        }

      /* If the previous character was a newline, count it. */
      if ((char *) p <= end && p[-1] == eol)
        {
          if (count)
            ++*count;

          if (d->mb_cur_max > 1)
            prepare_wc_buf ((const char *) p, end);
        }

      /* Check if we've run off the end of the buffer. */
      if ((char *) p > end)
        {
          if (d->mb_cur_max > 1)
            {
              free(mblen_buf);
              free(inputwcs);
            }
          *end = saved_end;
          return NULL;
        }

      if (s >= 0)
        {
          build_state(s, d);
          trans = d->trans;
          continue;
        }

      if (p[-1] == eol && allow_nl)
        {
          s = d->newlines[s1];
          continue;
        }

      s = 0;
    }
}