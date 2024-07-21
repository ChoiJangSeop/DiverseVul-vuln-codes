getpattern (value, quoted, expandpat)
     char *value;
     int quoted, expandpat;
{
  char *pat, *tword;
  WORD_LIST *l;
#if 0
  int i;
#endif
  /* There is a problem here:  how to handle single or double quotes in the
     pattern string when the whole expression is between double quotes?
     POSIX.2 says that enclosing double quotes do not cause the pattern to
     be quoted, but does that leave us a problem with @ and array[@] and their
     expansions inside a pattern? */
#if 0
  if (expandpat && (quoted & (Q_HERE_DOCUMENT|Q_DOUBLE_QUOTES)) && *tword)
    {
      i = 0;
      pat = string_extract_double_quoted (tword, &i, 1);
      free (tword);
      tword = pat;
    }
#endif

  /* expand_string_for_rhs () leaves WORD quoted and does not perform
     word splitting. */
  l = *value ? expand_string_for_rhs (value,
				      (quoted & (Q_HERE_DOCUMENT|Q_DOUBLE_QUOTES)) ? Q_PATQUOTE : quoted,
				      (int *)NULL, (int *)NULL)
	     : (WORD_LIST *)0;
  pat = string_list (l);
  dispose_words (l);
  if (pat)
    {
      tword = quote_string_for_globbing (pat, QGLOB_CVTNULL);
      free (pat);
      pat = tword;
    }
  return (pat);
}