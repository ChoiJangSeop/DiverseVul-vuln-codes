match_mb_charset (struct dfa *d, int s, position pos, int idx)
{
  int i;
  int match;		/* Flag which represent that matching succeed.  */
  int match_len;	/* Length of the character (or collating element)
                           with which this operator match.  */
  int op_len;		/* Length of the operator.  */
  char buffer[128];
  wchar_t wcbuf[6];

  /* Pointer to the structure to which we are currently refering.  */
  struct mb_char_classes *work_mbc;

  int context;
  wchar_t wc;		/* Current refering character.  */

  wc = inputwcs[idx];

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

  /* Assign the current refering operator to work_mbc.  */
  work_mbc = &(d->mbcsets[(d->multibyte_prop[pos.index]) >> 2]);
  match = !work_mbc->invert;
  match_len = (mblen_buf[idx] == 0)? 1 : mblen_buf[idx];

  /* Match in range 0-255?  */
  if (wc < NOTCHAR && work_mbc->cset != -1
      && tstbit((unsigned char)wc, d->charclasses[work_mbc->cset]))
    goto charset_matched;

  /* match with a character class?  */
  for (i = 0; i<work_mbc->nch_classes; i++)
    {
      if (iswctype((wint_t)wc, work_mbc->ch_classes[i]))
        goto charset_matched;
    }

  strncpy(buffer, (char const *) buf_begin + idx, match_len);
  buffer[match_len] = '\0';

  /* match with an equivalent class?  */
  for (i = 0; i<work_mbc->nequivs; i++)
    {
      op_len = strlen(work_mbc->equivs[i]);
      strncpy(buffer, (char const *) buf_begin + idx, op_len);
      buffer[op_len] = '\0';
      if (strcoll(work_mbc->equivs[i], buffer) == 0)
        {
          match_len = op_len;
          goto charset_matched;
        }
    }

  /* match with a collating element?  */
  for (i = 0; i<work_mbc->ncoll_elems; i++)
    {
      op_len = strlen(work_mbc->coll_elems[i]);
      strncpy(buffer, (char const *) buf_begin + idx, op_len);
      buffer[op_len] = '\0';

      if (strcoll(work_mbc->coll_elems[i], buffer) == 0)
        {
          match_len = op_len;
          goto charset_matched;
        }
    }

  wcbuf[0] = wc;
  wcbuf[1] = wcbuf[3] = wcbuf[5] = '\0';

  /* match with a range?  */
  for (i = 0; i<work_mbc->nranges; i++)
    {
      wcbuf[2] = work_mbc->range_sts[i];
      wcbuf[4] = work_mbc->range_ends[i];

      if (wcscoll(wcbuf, wcbuf+2) >= 0 &&
          wcscoll(wcbuf+4, wcbuf) >= 0)
        goto charset_matched;
    }

  /* match with a character?  */
  for (i = 0; i<work_mbc->nchars; i++)
    {
      if (wc == work_mbc->chars[i])
        goto charset_matched;
    }

  match = !match;

 charset_matched:
  return match ? match_len : 0;
}