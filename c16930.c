string_list_dollar_at (list, quoted, flags)
     WORD_LIST *list;
     int quoted;
     int flags;
{
  char *ifs, *ret;
#if defined (HANDLE_MULTIBYTE)
#  if defined (__GNUC__)
  char sep[MB_CUR_MAX + 1];
#  else
  char *sep = 0;
#  endif /* !__GNUC__ */
#else
  char sep[2];
#endif
  WORD_LIST *tlist;

  /* XXX this could just be ifs = ifs_value; */
  ifs = ifs_var ? value_cell (ifs_var) : (char *)0;

#if defined (HANDLE_MULTIBYTE)
#  if !defined (__GNUC__)
  sep = (char *)xmalloc (MB_CUR_MAX + 1);
#  endif /* !__GNUC__ */
  if (ifs && *ifs)
    {
      if (ifs_firstc_len == 1)
	{
	  sep[0] = ifs_firstc[0];
	  sep[1] = '\0';
	}
      else
	{
	  memcpy (sep, ifs_firstc, ifs_firstc_len);
	  sep[ifs_firstc_len] = '\0';
	}
    }
  else
    {
      sep[0] = ' ';
      sep[1] = '\0';
    }
#else
  sep[0] = (ifs == 0 || *ifs == 0) ? ' ' : *ifs;
  sep[1] = '\0';
#endif

  /* XXX -- why call quote_list if ifs == 0?  we can get away without doing
     it now that quote_escapes quotes spaces */
  tlist = (quoted & (Q_HERE_DOCUMENT|Q_DOUBLE_QUOTES|Q_PATQUOTE))
		? quote_list (list)
		: list_quote_escapes (list);

  ret = string_list_internal (tlist, sep);
#if defined (HANDLE_MULTIBYTE) && !defined (__GNUC__)
  free (sep);
#endif
  return ret;
}