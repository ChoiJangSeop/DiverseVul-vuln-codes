parameter_brace_expand_rhs (name, value, c, quoted, pflags, qdollaratp, hasdollarat)
     char *name, *value;
     int c, quoted, pflags, *qdollaratp, *hasdollarat;
{
  WORD_DESC *w;
  WORD_LIST *l;
  char *t, *t1, *temp, *vname;
  int l_hasdollat, sindex;

/*itrace("parameter_brace_expand_rhs: %s:%s pflags = %d", name, value, pflags);*/
  /* If the entire expression is between double quotes, we want to treat
     the value as a double-quoted string, with the exception that we strip
     embedded unescaped double quotes (for sh backwards compatibility). */
  if ((quoted & (Q_HERE_DOCUMENT|Q_DOUBLE_QUOTES)) && *value)
    {
      sindex = 0;
      temp = string_extract_double_quoted (value, &sindex, 1);
    }
  else
    temp = value;

  w = alloc_word_desc ();
  l_hasdollat = 0;
  /* XXX was 0 not quoted */
  l = *temp ? expand_string_for_rhs (temp, quoted, &l_hasdollat, (int *)NULL)
	    : (WORD_LIST *)0;
  if (hasdollarat)
    *hasdollarat = l_hasdollat || (l && l->next);
  if (temp != value)
    free (temp);
  if (l)
    {
      /* If l->next is not null, we know that TEMP contained "$@", since that
	 is the only expansion that creates more than one word. */
      if (qdollaratp && ((l_hasdollat && quoted) || l->next))
	{
/*itrace("parameter_brace_expand_rhs: %s:%s: l != NULL, set *qdollaratp", name, value);*/
	  *qdollaratp = 1;
	}

      /* The expansion of TEMP returned something.  We need to treat things
	  slightly differently if L_HASDOLLAT is non-zero.  If we have "$@",
	  the individual words have already been quoted.  We need to turn them
	  into a string with the words separated by the first character of
	  $IFS without any additional quoting, so string_list_dollar_at won't
	  do the right thing.  If IFS is null, we want "$@" to split into
	  separate arguments, not be concatenated, so we use string_list_internal
	  and mark the word to be split on spaces later.  We use
	  string_list_dollar_star for "$@" otherwise. */
      if (l->next && ifs_is_null)
	{
	  temp = string_list_internal (l, " ");
	  w->flags |= W_SPLITSPACE;
	}
      else
	temp = (l_hasdollat || l->next) ? string_list_dollar_star (l) : string_list (l);

      /* If we have a quoted null result (QUOTED_NULL(temp)) and the word is
	 a quoted null (l->next == 0 && QUOTED_NULL(l->word->word)), the
	 flags indicate it (l->word->flags & W_HASQUOTEDNULL), and the
	 expansion is quoted (quoted & (Q_HERE_DOCUMENT|Q_DOUBLE_QUOTES))
	 (which is more paranoia than anything else), we need to return the
	 quoted null string and set the flags to indicate it. */
      if (l->next == 0 && (quoted & (Q_HERE_DOCUMENT|Q_DOUBLE_QUOTES)) && QUOTED_NULL (temp) && QUOTED_NULL (l->word->word) && (l->word->flags & W_HASQUOTEDNULL))
	{
	  w->flags |= W_HASQUOTEDNULL;
/*itrace("parameter_brace_expand_rhs (%s:%s): returning quoted null, turning off qdollaratp", name, value);*/
	  /* If we return a quoted null with L_HASDOLLARAT, we either have a
	     construct like "${@-$@}" or "${@-${@-$@}}" with no positional
	     parameters or a quoted expansion of "$@" with $1 == ''.  In either
	     case, we don't want to enable special handling of $@. */
	  if (qdollaratp && l_hasdollat)
	    *qdollaratp = 0;
	}
      dispose_words (l);
    }
  else if ((quoted & (Q_HERE_DOCUMENT|Q_DOUBLE_QUOTES)) && l_hasdollat)
    {
      /* Posix interp 221 changed the rules on this.  The idea is that
	 something like "$xxx$@" should expand the same as "${foo-$xxx$@}"
	 when foo and xxx are unset.  The problem is that it's not in any
	 way backwards compatible and few other shells do it.  We're eventually
	 going to try and split the difference (heh) a little bit here. */
      /* l_hasdollat == 1 means we saw a quoted dollar at.  */

      /* The brace expansion occurred between double quotes and there was
	 a $@ in TEMP.  It does not matter if the $@ is quoted, as long as
	 it does not expand to anything.  In this case, we want to return
	 a quoted empty string.  Posix interp 888 */
      temp = make_quoted_char ('\0');
      w->flags |= W_HASQUOTEDNULL;
/*itrace("parameter_brace_expand_rhs (%s:%s): returning quoted null", name, value);*/
    }
  else
    temp = (char *)NULL;

  if (c == '-' || c == '+')
    {
      w->word = temp;
      return w;
    }

  /* c == '=' */
  t = temp ? savestring (temp) : savestring ("");
  t1 = dequote_string (t);
  free (t);

  /* bash-4.4/5.0 */
  vname = name;
  if (*name == '!' &&
      (legal_variable_starter ((unsigned char)name[1]) || DIGIT (name[1]) || VALID_INDIR_PARAM (name[1])))
    {
      vname = parameter_brace_find_indir (name + 1, SPECIAL_VAR (name, 1), quoted, 1);
      if (vname == 0 || *vname == 0)
	{
	  report_error (_("%s: invalid indirect expansion"), name);
	  free (vname);
	  dispose_word (w);
	  return &expand_wdesc_error;
	}
      if (legal_identifier (vname) == 0)
	{
	  report_error (_("%s: invalid variable name"), vname);
	  free (vname);
	  dispose_word (w);
	  return &expand_wdesc_error;
	}
    }
    
#if defined (ARRAY_VARS)
  if (valid_array_reference (vname, 0))
    assign_array_element (vname, t1, 0);
  else
#endif /* ARRAY_VARS */
  bind_variable (vname, t1, 0);

  stupidly_hack_special_variables (vname);

  if (vname != name)
    free (vname);

  /* From Posix group discussion Feb-March 2010.  Issue 7 0000221 */
  free (temp);

  w->word = t1;
  return w;
}