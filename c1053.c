parse_bracket_exp (void)
{
  int invert;
  int c, c1, c2;
  charclass ccl;

  /* Used to warn about [:space:].
     Bit 0 = first character is a colon.
     Bit 1 = last character is a colon.
     Bit 2 = includes any other character but a colon.
     Bit 3 = includes ranges, char/equiv classes or collation elements.  */
  int colon_warning_state;

  wint_t wc;
  wint_t wc2;
  wint_t wc1 = 0;

  /* Work area to build a mb_char_classes.  */
  struct mb_char_classes *work_mbc;
  int chars_al, range_sts_al, range_ends_al, ch_classes_al,
    equivs_al, coll_elems_al;

  chars_al = 0;
  range_sts_al = range_ends_al = 0;
  ch_classes_al = equivs_al = coll_elems_al = 0;
  if (MB_CUR_MAX > 1)
    {
      REALLOC_IF_NECESSARY(dfa->mbcsets, dfa->mbcsets_alloc, dfa->nmbcsets + 1);

      /* dfa->multibyte_prop[] hold the index of dfa->mbcsets.
         We will update dfa->multibyte_prop[] in addtok(), because we can't
         decide the index in dfa->tokens[].  */

      /* Initialize work area.  */
      work_mbc = &(dfa->mbcsets[dfa->nmbcsets++]);
      memset (work_mbc, 0, sizeof *work_mbc);
    }
  else
    work_mbc = NULL;

  memset (ccl, 0, sizeof ccl);
  FETCH_WC (c, wc, _("unbalanced ["));
  if (c == '^')
    {
      FETCH_WC (c, wc, _("unbalanced ["));
      invert = 1;
    }
  else
    invert = 0;

  colon_warning_state = (c == ':');
  do
    {
      c1 = EOF; /* mark c1 is not initialized".  */
      colon_warning_state &= ~2;

      /* Note that if we're looking at some other [:...:] construct,
         we just treat it as a bunch of ordinary characters.  We can do
         this because we assume regex has checked for syntax errors before
         dfa is ever called. */
      if (c == '[' && (syntax_bits & RE_CHAR_CLASSES))
        {
#define BRACKET_BUFFER_SIZE 128
          char str[BRACKET_BUFFER_SIZE];
          FETCH_WC (c1, wc1, _("unbalanced ["));

          /* If pattern contains `[[:', `[[.', or `[[='.  */
          if (c1 == ':'
              /* TODO: handle `[[.' and `[[=' also for MB_CUR_MAX == 1.  */
              || (MB_CUR_MAX > 1 && (c1 == '.' || c1 == '='))
              )
            {
              size_t len = 0;
              for (;;)
                {
                  FETCH_WC (c, wc, _("unbalanced ["));
                  if ((c == c1 && *lexptr == ']') || lexleft == 0)
                    break;
                  if (len < BRACKET_BUFFER_SIZE)
                    str[len++] = c;
                  else
                    /* This is in any case an invalid class name.  */
                    str[0] = '\0';
                }
              str[len] = '\0';

              /* Fetch bracket.  */
              FETCH_WC (c, wc, _("unbalanced ["));
              if (c1 == ':')
                /* build character class.  */
                {
                  char const *class
                    = (case_fold && (STREQ  (str, "upper")
                                     || STREQ  (str, "lower"))
                                       ? "alpha"
                                       : str);
                  const struct dfa_ctype *pred = find_pred (class);
                  if (!pred)
                    dfaerror(_("invalid character class"));

                  if (MB_CUR_MAX > 1 && !pred->single_byte_only)
                    {
                      /* Store the character class as wctype_t.  */
                      wctype_t wt = wctype (class);

                      REALLOC_IF_NECESSARY(work_mbc->ch_classes,
                                           ch_classes_al,
                                           work_mbc->nch_classes + 1);
                      work_mbc->ch_classes[work_mbc->nch_classes++] = wt;
                    }

                  for (c2 = 0; c2 < NOTCHAR; ++c2)
                    if (pred->func(c2))
                      setbit_case_fold_c (c2, ccl);
                }

              else if (MBS_SUPPORT && (c1 == '=' || c1 == '.'))
                {
                  char *elem;
                  MALLOC(elem, len + 1);
                  strncpy(elem, str, len + 1);

                  if (c1 == '=')
                    /* build equivalent class.  */
                    {
                      REALLOC_IF_NECESSARY(work_mbc->equivs,
                                           equivs_al,
                                           work_mbc->nequivs + 1);
                      work_mbc->equivs[work_mbc->nequivs++] = elem;
                    }

                  if (c1 == '.')
                    /* build collating element.  */
                    {
                      REALLOC_IF_NECESSARY(work_mbc->coll_elems,
                                           coll_elems_al,
                                           work_mbc->ncoll_elems + 1);
                      work_mbc->coll_elems[work_mbc->ncoll_elems++] = elem;
                    }
                }
              colon_warning_state |= 8;

              /* Fetch new lookahead character.  */
              FETCH_WC (c1, wc1, _("unbalanced ["));
              continue;
            }

          /* We treat '[' as a normal character here.  c/c1/wc/wc1
             are already set up.  */
        }

      if (c == '\\' && (syntax_bits & RE_BACKSLASH_ESCAPE_IN_LISTS))
        FETCH_WC(c, wc, _("unbalanced ["));

      if (c1 == EOF)
        FETCH_WC(c1, wc1, _("unbalanced ["));

      if (c1 == '-')
        /* build range characters.  */
        {
          FETCH_WC(c2, wc2, _("unbalanced ["));
          if (c2 == ']')
            {
              /* In the case [x-], the - is an ordinary hyphen,
                 which is left in c1, the lookahead character. */
              lexptr -= cur_mb_len;
              lexleft += cur_mb_len;
            }
        }

      if (c1 == '-' && c2 != ']')
        {
          if (c2 == '\\'
              && (syntax_bits & RE_BACKSLASH_ESCAPE_IN_LISTS))
            FETCH_WC(c2, wc2, _("unbalanced ["));

          if (MB_CUR_MAX > 1)
            {
              /* When case folding map a range, say [m-z] (or even [M-z])
                 to the pair of ranges, [m-z] [M-Z].  */
              REALLOC_IF_NECESSARY(work_mbc->range_sts,
                                   range_sts_al, work_mbc->nranges + 1);
              REALLOC_IF_NECESSARY(work_mbc->range_ends,
                                   range_ends_al, work_mbc->nranges + 1);
              work_mbc->range_sts[work_mbc->nranges] =
                case_fold ? towlower(wc) : (wchar_t)wc;
              work_mbc->range_ends[work_mbc->nranges++] =
                case_fold ? towlower(wc2) : (wchar_t)wc2;

#ifndef GREP
              if (case_fold && (iswalpha(wc) || iswalpha(wc2)))
                {
                  REALLOC_IF_NECESSARY(work_mbc->range_sts,
                                       range_sts_al, work_mbc->nranges + 1);
                  work_mbc->range_sts[work_mbc->nranges] = towupper(wc);
                  REALLOC_IF_NECESSARY(work_mbc->range_ends,
                                       range_ends_al, work_mbc->nranges + 1);
                  work_mbc->range_ends[work_mbc->nranges++] = towupper(wc2);
                }
#endif
            }
          else
            {
              c1 = c;
              if (case_fold)
                {
                  c1 = tolower (c1);
                  c2 = tolower (c2);
                }
              if (!hard_LC_COLLATE)
                for (c = c1; c <= c2; c++)
                  setbit_case_fold_c (c, ccl);
              else
                {
                  /* Defer to the system regex library about the meaning
                     of range expressions.  */
                  regex_t re;
                  char pattern[6] = { '[', c1, '-', c2, ']', 0 };
                  char subject[2] = { 0, 0 };
                  regcomp (&re, pattern, REG_NOSUB);
                  for (c = 0; c < NOTCHAR; ++c)
                    {
                      subject[0] = c;
                      if (!(case_fold && isupper (c))
                          && regexec (&re, subject, 0, NULL, 0) != REG_NOMATCH)
                        setbit_case_fold_c (c, ccl);
                    }
                  regfree (&re);
                }
            }

          colon_warning_state |= 8;
          FETCH_WC(c1, wc1, _("unbalanced ["));
          continue;
        }

      colon_warning_state |= (c == ':') ? 2 : 4;

      if (MB_CUR_MAX == 1)
        {
          setbit_case_fold_c (c, ccl);
          continue;
        }

      if (case_fold && iswalpha(wc))
        {
          wc = towlower(wc);
          if (!setbit_wc (wc, ccl))
            {
              REALLOC_IF_NECESSARY(work_mbc->chars, chars_al,
                                   work_mbc->nchars + 1);
              work_mbc->chars[work_mbc->nchars++] = wc;
            }
#ifdef GREP
          continue;
#else
          wc = towupper(wc);
#endif
        }
      if (!setbit_wc (wc, ccl))
        {
          REALLOC_IF_NECESSARY(work_mbc->chars, chars_al,
                               work_mbc->nchars + 1);
          work_mbc->chars[work_mbc->nchars++] = wc;
        }
    }
  while ((wc = wc1, (c = c1) != ']'));

  if (colon_warning_state == 7)
    dfawarn (_("character class syntax is [[:space:]], not [:space:]"));

  if (MB_CUR_MAX > 1)
    {
      static charclass zeroclass;
      work_mbc->invert = invert;
      work_mbc->cset = equal(ccl, zeroclass) ? -1 : charclass_index(ccl);
      return MBCSET;
    }

  if (invert)
    {
      assert(MB_CUR_MAX == 1);
      notset(ccl);
      if (syntax_bits & RE_HAT_LISTS_NOT_NEWLINE)
        clrbit(eolbyte, ccl);
    }

  return CSET + charclass_index(ccl);
}