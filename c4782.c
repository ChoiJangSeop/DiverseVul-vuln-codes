static WORD_LIST *
expand_word_internal (word, quoted, isexp, contains_dollar_at, expanded_something)
     WORD_DESC *word;
     int quoted, isexp;
     int *contains_dollar_at;
     int *expanded_something;
{
  WORD_LIST *list;
  WORD_DESC *tword;

  /* The intermediate string that we build while expanding. */
  char *istring;

  /* The current size of the above object. */
  size_t istring_size;

  /* Index into ISTRING. */
  int istring_index;

  /* Temporary string storage. */
  char *temp, *temp1;

  /* The text of WORD. */
  register char *string;

  /* The size of STRING. */
  size_t string_size;

  /* The index into STRING. */
  int sindex;

  /* This gets 1 if we see a $@ while quoted. */
  int quoted_dollar_at;

  /* One of UNQUOTED, PARTIALLY_QUOTED, or WHOLLY_QUOTED, depending on
     whether WORD contains no quoting characters, a partially quoted
     string (e.g., "xx"ab), or is fully quoted (e.g., "xxab"). */
  int quoted_state;

  /* State flags */
  int had_quoted_null;
  int has_dollar_at, temp_has_dollar_at;
  int split_on_spaces;
  int tflag;
  int pflags;			/* flags passed to param_expand */
  int mb_cur_max;

  int assignoff;		/* If assignment, offset of `=' */

  register unsigned char c;	/* Current character. */
  int t_index;			/* For calls to string_extract_xxx. */

  char twochars[2];

  DECLARE_MBSTATE;

  /* OK, let's see if we can optimize a common idiom: "$@" */
  if (STREQ (word->word, "\"$@\"") &&
      (word->flags == (W_HASDOLLAR|W_QUOTED)) &&
      dollar_vars[1])		/* XXX - check IFS here as well? */
    {
      if (contains_dollar_at)
	*contains_dollar_at = 1;
      if (expanded_something)
	*expanded_something = 1;
      if (cached_quoted_dollar_at)
	return (copy_word_list (cached_quoted_dollar_at));
      list = list_rest_of_args ();
      list = quote_list (list);
      cached_quoted_dollar_at = copy_word_list (list);
      return (list);
    }

  istring = (char *)xmalloc (istring_size = DEFAULT_INITIAL_ARRAY_SIZE);
  istring[istring_index = 0] = '\0';
  quoted_dollar_at = had_quoted_null = has_dollar_at = 0;
  split_on_spaces = 0;
  quoted_state = UNQUOTED;

  string = word->word;
  if (string == 0)
    goto finished_with_string;
  mb_cur_max = MB_CUR_MAX;

  /* Don't need the string length for the SADD... and COPY_ macros unless
     multibyte characters are possible. */
  string_size = (mb_cur_max > 1) ? strlen (string) : 1;

  if (contains_dollar_at)
    *contains_dollar_at = 0;

  assignoff = -1;

  /* Begin the expansion. */

  for (sindex = 0; ;)
    {
      c = string[sindex];

      /* Case on top-level character. */
      switch (c)
	{
	case '\0':
	  goto finished_with_string;

	case CTLESC:
	  sindex++;
#if HANDLE_MULTIBYTE
	  if (mb_cur_max > 1 && string[sindex])
	    {
	      SADD_MBQCHAR_BODY(temp, string, sindex, string_size);
	    }
	  else
#endif
	    {
	      temp = (char *)xmalloc (3);
	      temp[0] = CTLESC;
	      temp[1] = c = string[sindex];
	      temp[2] = '\0';
	    }

dollar_add_string:
	  if (string[sindex])
	    sindex++;

add_string:
	  if (temp)
	    {
	      istring = sub_append_string (temp, istring, &istring_index, &istring_size);
	      temp = (char *)0;
	    }

	  break;

#if defined (PROCESS_SUBSTITUTION)
	  /* Process substitution. */
	case '<':
	case '>':
	  {
	    /* bash-4.4/bash-5.0
	       XXX - technically this should only be expanded at the start
	       of a word */
	    if (string[++sindex] != LPAREN || (quoted & (Q_HERE_DOCUMENT|Q_DOUBLE_QUOTES)) || (word->flags & (W_DQUOTE|W_NOPROCSUB)) || posixly_correct)
	      {
		sindex--;	/* add_character: label increments sindex */
		goto add_character;
	      }
	    else
	      t_index = sindex + 1; /* skip past both '<' and LPAREN */

	    temp1 = extract_process_subst (string, (c == '<') ? "<(" : ">(", &t_index, 0); /*))*/
	    sindex = t_index;

	    /* If the process substitution specification is `<()', we want to
	       open the pipe for writing in the child and produce output; if
	       it is `>()', we want to open the pipe for reading in the child
	       and consume input. */
	    temp = temp1 ? process_substitute (temp1, (c == '>')) : (char *)0;

	    FREE (temp1);

	    goto dollar_add_string;
	  }
#endif /* PROCESS_SUBSTITUTION */

	case '=':
	  /* Posix.2 section 3.6.1 says that tildes following `=' in words
	     which are not assignment statements are not expanded.  If the
	     shell isn't in posix mode, though, we perform tilde expansion
	     on `likely candidate' unquoted assignment statements (flags
	     include W_ASSIGNMENT but not W_QUOTED).  A likely candidate
	     contains an unquoted :~ or =~.  Something to think about: we
	     now have a flag that says  to perform tilde expansion on arguments
	     to `assignment builtins' like declare and export that look like
	     assignment statements.  We now do tilde expansion on such words
	     even in POSIX mode. */	
	  if (word->flags & (W_ASSIGNRHS|W_NOTILDE))
	    {
	      if (isexp == 0 && (word->flags & (W_NOSPLIT|W_NOSPLIT2)) == 0 && isifs (c))
		goto add_ifs_character;
	      else
		goto add_character;
	    }
	  /* If we're not in posix mode or forcing assignment-statement tilde
	     expansion, note where the `=' appears in the word and prepare to
	     do tilde expansion following the first `='. */
	  if ((word->flags & W_ASSIGNMENT) &&
	      (posixly_correct == 0 || (word->flags & W_TILDEEXP)) &&
	      assignoff == -1 && sindex > 0)
	    assignoff = sindex;
	  if (sindex == assignoff && string[sindex+1] == '~')	/* XXX */
	    word->flags |= W_ITILDE;
#if 0
	  else if ((word->flags & W_ASSIGNMENT) &&
		   (posixly_correct == 0 || (word->flags & W_TILDEEXP)) &&
		   string[sindex+1] == '~')
	    word->flags |= W_ITILDE;
#endif

	  /* XXX - bash-4.4/bash-5.0 */
	  if (word->flags & W_ASSIGNARG)
	    word->flags |= W_ASSIGNRHS;		/* affects $@ */

	  if (isexp == 0 && (word->flags & (W_NOSPLIT|W_NOSPLIT2)) == 0 && isifs (c))
	    goto add_ifs_character;
	  else
	    goto add_character;

	case ':':
	  if (word->flags & W_NOTILDE)
	    {
	      if (isexp == 0 && (word->flags & (W_NOSPLIT|W_NOSPLIT2)) == 0 && isifs (c))
		goto add_ifs_character;
	      else
		goto add_character;
	    }

	  if ((word->flags & (W_ASSIGNMENT|W_ASSIGNRHS|W_TILDEEXP)) &&
	      string[sindex+1] == '~')
	    word->flags |= W_ITILDE;

	  if (isexp == 0 && (word->flags & (W_NOSPLIT|W_NOSPLIT2)) == 0 && isifs (c))
	    goto add_ifs_character;
	  else
	    goto add_character;

	case '~':
	  /* If the word isn't supposed to be tilde expanded, or we're not
	     at the start of a word or after an unquoted : or = in an
	     assignment statement, we don't do tilde expansion.  If we don't want
	     tilde expansion when expanding words to be passed to the arithmetic
	     evaluator, remove the check for Q_ARITH. */
	  if ((word->flags & (W_NOTILDE|W_DQUOTE)) ||
	      (sindex > 0 && ((word->flags & W_ITILDE) == 0)) ||
	      ((quoted & (Q_DOUBLE_QUOTES|Q_HERE_DOCUMENT)) && ((quoted & Q_ARITH) == 0)))
	    {
	      word->flags &= ~W_ITILDE;
	      if (isexp == 0 && (word->flags & (W_NOSPLIT|W_NOSPLIT2)) == 0 && isifs (c) && (quoted & (Q_DOUBLE_QUOTES|Q_HERE_DOCUMENT)) == 0)
		goto add_ifs_character;
	      else
		goto add_character;
	    }

	  if (word->flags & W_ASSIGNRHS)
	    tflag = 2;
	  else if (word->flags & (W_ASSIGNMENT|W_TILDEEXP))
	    tflag = 1;
	  else
	    tflag = 0;

	  temp = bash_tilde_find_word (string + sindex, tflag, &t_index);
	    
	  word->flags &= ~W_ITILDE;

	  if (temp && *temp && t_index > 0)
	    {
	      temp1 = bash_tilde_expand (temp, tflag);
	      if  (temp1 && *temp1 == '~' && STREQ (temp, temp1))
		{
		  FREE (temp);
		  FREE (temp1);
		  goto add_character;		/* tilde expansion failed */
		}
	      free (temp);
	      temp = temp1;
	      sindex += t_index;
	      goto add_quoted_string;		/* XXX was add_string */
	    }
	  else
	    {
	      FREE (temp);
	      goto add_character;
	    }
	
	case '$':
	  if (expanded_something)
	    *expanded_something = 1;

	  temp_has_dollar_at = 0;
	  pflags = (word->flags & W_NOCOMSUB) ? PF_NOCOMSUB : 0;
	  if (word->flags & W_NOSPLIT2)
	    pflags |= PF_NOSPLIT2;
	  if (word->flags & W_ASSIGNRHS)
	    pflags |= PF_ASSIGNRHS;
	  if (word->flags & W_COMPLETE)
	    pflags |= PF_COMPLETE;
	  tword = param_expand (string, &sindex, quoted, expanded_something,
			       &temp_has_dollar_at, &quoted_dollar_at,
			       &had_quoted_null, pflags);
	  has_dollar_at += temp_has_dollar_at;
	  split_on_spaces += (tword->flags & W_SPLITSPACE);

	  if (tword == &expand_wdesc_error || tword == &expand_wdesc_fatal)
	    {
	      free (string);
	      free (istring);
	      return ((tword == &expand_wdesc_error) ? &expand_word_error
						     : &expand_word_fatal);
	    }
	  if (contains_dollar_at && has_dollar_at)
	    *contains_dollar_at = 1;

	  if (tword && (tword->flags & W_HASQUOTEDNULL))
	    had_quoted_null = 1;

	  temp = tword ? tword->word : (char *)NULL;
	  dispose_word_desc (tword);

	  /* Kill quoted nulls; we will add them back at the end of
	     expand_word_internal if nothing else in the string */
	  if (had_quoted_null && temp && QUOTED_NULL (temp))
	    {
	      FREE (temp);
	      temp = (char *)NULL;
	    }

	  goto add_string;
	  break;

	case '`':		/* Backquoted command substitution. */
	  {
	    t_index = sindex++;

	    temp = string_extract (string, &sindex, "`", SX_REQMATCH);
	    /* The test of sindex against t_index is to allow bare instances of
	       ` to pass through, for backwards compatibility. */
	    if (temp == &extract_string_error || temp == &extract_string_fatal)
	      {
		if (sindex - 1 == t_index)
		  {
		    sindex = t_index;
		    goto add_character;
		  }
		last_command_exit_value = EXECUTION_FAILURE;
		report_error (_("bad substitution: no closing \"`\" in %s") , string+t_index);
		free (string);
		free (istring);
		return ((temp == &extract_string_error) ? &expand_word_error
							: &expand_word_fatal);
	      }
		
	    if (expanded_something)
	      *expanded_something = 1;

	    if (word->flags & W_NOCOMSUB)
	      /* sindex + 1 because string[sindex] == '`' */
	      temp1 = substring (string, t_index, sindex + 1);
	    else
	      {
		de_backslash (temp);
		tword = command_substitute (temp, quoted);
		temp1 = tword ? tword->word : (char *)NULL;
		if (tword)
		  dispose_word_desc (tword);
	      }
	    FREE (temp);
	    temp = temp1;
	    goto dollar_add_string;
	  }

	case '\\':
	  if (string[sindex + 1] == '\n')
	    {
	      sindex += 2;
	      continue;
	    }

	  c = string[++sindex];

	  if (quoted & Q_HERE_DOCUMENT)
	    tflag = CBSHDOC;
	  else if (quoted & Q_DOUBLE_QUOTES)
	    tflag = CBSDQUOTE;
	  else
	    tflag = 0;

	  /* From Posix discussion on austin-group list:  Backslash escaping
	     a } in ${...} is removed.  Issue 0000221 */
	  if ((quoted & Q_DOLBRACE) && c == RBRACE)
	    {
	      SCOPY_CHAR_I (twochars, CTLESC, c, string, sindex, string_size);
	    }
	  /* This is the fix for " $@\ " */
	  else if ((quoted & (Q_HERE_DOCUMENT|Q_DOUBLE_QUOTES)) && ((sh_syntaxtab[c] & tflag) == 0) && isexp == 0 && isifs (c))
	    {
	      RESIZE_MALLOCED_BUFFER (istring, istring_index, 2, istring_size,
				      DEFAULT_ARRAY_SIZE);
	      istring[istring_index++] = CTLESC;
	      istring[istring_index++] = '\\';
	      istring[istring_index] = '\0';

	      SCOPY_CHAR_I (twochars, CTLESC, c, string, sindex, string_size);
	    }
	  else if ((quoted & (Q_HERE_DOCUMENT|Q_DOUBLE_QUOTES)) && ((sh_syntaxtab[c] & tflag) == 0))
	    {
	      SCOPY_CHAR_I (twochars, '\\', c, string, sindex, string_size);
	    }
	  else if (c == 0)
	    {
	      c = CTLNUL;
	      sindex--;		/* add_character: label increments sindex */
	      goto add_character;
	    }
	  else
	    {
	      SCOPY_CHAR_I (twochars, CTLESC, c, string, sindex, string_size);
	    }

	  sindex++;
add_twochars:
	  /* BEFORE jumping here, we need to increment sindex if appropriate */
	  RESIZE_MALLOCED_BUFFER (istring, istring_index, 2, istring_size,
				  DEFAULT_ARRAY_SIZE);
	  istring[istring_index++] = twochars[0];
	  istring[istring_index++] = twochars[1];
	  istring[istring_index] = '\0';

	  break;

	case '"':
	  if ((quoted & (Q_DOUBLE_QUOTES|Q_HERE_DOCUMENT)) && ((quoted & Q_ARITH) == 0))
	    goto add_character;

	  t_index = ++sindex;
	  temp = string_extract_double_quoted (string, &sindex, (word->flags & W_COMPLETE) ? SX_COMPLETE : 0);

	  /* If the quotes surrounded the entire string, then the
	     whole word was quoted. */
	  quoted_state = (t_index == 1 && string[sindex] == '\0')
			    ? WHOLLY_QUOTED
			    : PARTIALLY_QUOTED;

	  if (temp && *temp)
	    {
	      tword = alloc_word_desc ();
	      tword->word = temp;

	      /* XXX - bash-4.4/bash-5.0 */
	      if (word->flags & W_ASSIGNARG)
		tword->flags |= word->flags & (W_ASSIGNARG|W_ASSIGNRHS);	/* affects $@ */
	      if (word->flags & W_COMPLETE)
		tword->flags |= W_COMPLETE;	/* for command substitutions */

	      temp = (char *)NULL;

	      temp_has_dollar_at = 0;	/* XXX */
	      /* Need to get W_HASQUOTEDNULL flag through this function. */
	      list = expand_word_internal (tword, Q_DOUBLE_QUOTES, 0, &temp_has_dollar_at, (int *)NULL);
	      has_dollar_at += temp_has_dollar_at;

	      if (list == &expand_word_error || list == &expand_word_fatal)
		{
		  free (istring);
		  free (string);
		  /* expand_word_internal has already freed temp_word->word
		     for us because of the way it prints error messages. */
		  tword->word = (char *)NULL;
		  dispose_word (tword);
		  return list;
		}

	      dispose_word (tword);

	      /* "$@" (a double-quoted dollar-at) expands into nothing,
		 not even a NULL word, when there are no positional
		 parameters. */
	      if (list == 0 && temp_has_dollar_at)	/* XXX - was has_dollar_at */
		{
		  quoted_dollar_at++;
		  break;
		}

	      /* If we get "$@", we know we have expanded something, so we
		 need to remember it for the final split on $IFS.  This is
		 a special case; it's the only case where a quoted string
		 can expand into more than one word.  It's going to come back
		 from the above call to expand_word_internal as a list with
		 a single word, in which all characters are quoted and
		 separated by blanks.  What we want to do is to turn it back
		 into a list for the next piece of code. */
	      if (list)
		dequote_list (list);

	      if (list && list->word && (list->word->flags & W_HASQUOTEDNULL))
		had_quoted_null = 1;		/* XXX */

	      if (temp_has_dollar_at)		/* XXX - was has_dollar_at */
		{
		  quoted_dollar_at++;
		  if (contains_dollar_at)
		    *contains_dollar_at = 1;
		  if (expanded_something)
		    *expanded_something = 1;
		}
	    }
	  else
	    {
	      /* What we have is "".  This is a minor optimization. */
	      FREE (temp);
	      list = (WORD_LIST *)NULL;
	    }

	  /* The code above *might* return a list (consider the case of "$@",
	     where it returns "$1", "$2", etc.).  We can't throw away the
	     rest of the list, and we have to make sure each word gets added
	     as quoted.  We test on tresult->next:  if it is non-NULL, we
	     quote the whole list, save it to a string with string_list, and
	     add that string. We don't need to quote the results of this
	     (and it would be wrong, since that would quote the separators
	     as well), so we go directly to add_string. */
	  if (list)
	    {
	      if (list->next)
		{
		  /* Testing quoted_dollar_at makes sure that "$@" is
		     split correctly when $IFS does not contain a space. */
		  temp = quoted_dollar_at
				? string_list_dollar_at (list, Q_DOUBLE_QUOTES, 0)
				: string_list (quote_list (list));
		  dispose_words (list);
		  goto add_string;
		}
	      else
		{
		  temp = savestring (list->word->word);
		  tflag = list->word->flags;
		  dispose_words (list);

		  /* If the string is not a quoted null string, we want
		     to remove any embedded unquoted CTLNUL characters.
		     We do not want to turn quoted null strings back into
		     the empty string, though.  We do this because we
		     want to remove any quoted nulls from expansions that
		     contain other characters.  For example, if we have
		     x"$*"y or "x$*y" and there are no positional parameters,
		     the $* should expand into nothing. */
		  /* We use the W_HASQUOTEDNULL flag to differentiate the
		     cases:  a quoted null character as above and when
		     CTLNUL is contained in the (non-null) expansion
		     of some variable.  We use the had_quoted_null flag to
		     pass the value through this function to its caller. */
		  if ((tflag & W_HASQUOTEDNULL) && QUOTED_NULL (temp) == 0)
		    remove_quoted_nulls (temp);	/* XXX */
		}
	    }
	  else
	    temp = (char *)NULL;

	  /* We do not want to add quoted nulls to strings that are only
	     partially quoted; we can throw them away.  The exception to
	     this is when we are going to be performing word splitting,
	     since we have to preserve a null argument if the next character
	     will cause word splitting. */
	  if (temp == 0 && quoted_state == PARTIALLY_QUOTED && (word->flags & (W_NOSPLIT|W_NOSPLIT2)))
	    continue;

	add_quoted_string:

	  if (temp)
	    {
	      temp1 = temp;
	      temp = quote_string (temp);
	      free (temp1);
	      goto add_string;
	    }
	  else
	    {
	      /* Add NULL arg. */
	      c = CTLNUL;
	      sindex--;		/* add_character: label increments sindex */
	      goto add_character;
	    }

	  /* break; */

	case '\'':
	  if ((quoted & (Q_DOUBLE_QUOTES|Q_HERE_DOCUMENT)))
	    goto add_character;

	  t_index = ++sindex;
	  temp = string_extract_single_quoted (string, &sindex);

	  /* If the entire STRING was surrounded by single quotes,
	     then the string is wholly quoted. */
	  quoted_state = (t_index == 1 && string[sindex] == '\0')
			    ? WHOLLY_QUOTED
			    : PARTIALLY_QUOTED;

	  /* If all we had was '', it is a null expansion. */
	  if (*temp == '\0')
	    {
	      free (temp);
	      temp = (char *)NULL;
	    }
	  else
	    remove_quoted_escapes (temp);	/* ??? */

	  /* We do not want to add quoted nulls to strings that are only
	     partially quoted; such nulls are discarded.  See above for the
	     exception, which is when the string is going to be split. */
	  if (temp == 0 && (quoted_state == PARTIALLY_QUOTED) && (word->flags & (W_NOSPLIT|W_NOSPLIT2)))
	    continue;

	  /* If we have a quoted null expansion, add a quoted NULL to istring. */
	  if (temp == 0)
	    {
	      c = CTLNUL;
	      sindex--;		/* add_character: label increments sindex */
	      goto add_character;
	    }
	  else
	    goto add_quoted_string;

	  /* break; */

	default:
	  /* This is the fix for " $@ " */
	add_ifs_character:
	  if ((quoted & (Q_HERE_DOCUMENT|Q_DOUBLE_QUOTES)) || (isexp == 0 && isifs (c) && (word->flags & (W_NOSPLIT|W_NOSPLIT2)) == 0))
	    {
	      if (string[sindex])	/* from old goto dollar_add_string */
		sindex++;
	      if (c == 0)
		{
		  c = CTLNUL;
		  goto add_character;
		}
	      else
		{
#if HANDLE_MULTIBYTE
		  if (mb_cur_max > 1)
		    sindex--;

		  if (mb_cur_max > 1)
		    {
		      SADD_MBQCHAR_BODY(temp, string, sindex, string_size);
		    }
		  else
#endif
		    {
		      twochars[0] = CTLESC;
		      twochars[1] = c;
		      goto add_twochars;
		    }
		}
	    }

	  SADD_MBCHAR (temp, string, sindex, string_size);

	add_character:
	  RESIZE_MALLOCED_BUFFER (istring, istring_index, 1, istring_size,
				  DEFAULT_ARRAY_SIZE);
	  istring[istring_index++] = c;
	  istring[istring_index] = '\0';

	  /* Next character. */
	  sindex++;
	}
    }

finished_with_string:
  /* OK, we're ready to return.  If we have a quoted string, and
     quoted_dollar_at is not set, we do no splitting at all; otherwise
     we split on ' '.  The routines that call this will handle what to
     do if nothing has been expanded. */

  /* Partially and wholly quoted strings which expand to the empty
     string are retained as an empty arguments.  Unquoted strings
     which expand to the empty string are discarded.  The single
     exception is the case of expanding "$@" when there are no
     positional parameters.  In that case, we discard the expansion. */

  /* Because of how the code that handles "" and '' in partially
     quoted strings works, we need to make ISTRING into a QUOTED_NULL
     if we saw quoting characters, but the expansion was empty.
     "" and '' are tossed away before we get to this point when
     processing partially quoted strings.  This makes "" and $xxx""
     equivalent when xxx is unset.  We also look to see whether we
     saw a quoted null from a ${} expansion and add one back if we
     need to. */

  /* If we expand to nothing and there were no single or double quotes
     in the word, we throw it away.  Otherwise, we return a NULL word.
     The single exception is for $@ surrounded by double quotes when
     there are no positional parameters.  In that case, we also throw
     the word away. */

  if (*istring == '\0')
    {
      if (quoted_dollar_at == 0 && (had_quoted_null || quoted_state == PARTIALLY_QUOTED))
	{
	  istring[0] = CTLNUL;
	  istring[1] = '\0';
	  tword = make_bare_word (istring);
	  tword->flags |= W_HASQUOTEDNULL;		/* XXX */
	  list = make_word_list (tword, (WORD_LIST *)NULL);
	  if (quoted & (Q_HERE_DOCUMENT|Q_DOUBLE_QUOTES))
	    tword->flags |= W_QUOTED;
	}
      /* According to sh, ksh, and Posix.2, if a word expands into nothing
	 and a double-quoted "$@" appears anywhere in it, then the entire
	 word is removed. */
      /* XXX - exception appears to be that quoted null strings result in
	 null arguments */
      else  if (quoted_state == UNQUOTED || quoted_dollar_at)
	list = (WORD_LIST *)NULL;
#if 0
      else
	{
	  tword = make_bare_word (istring);
	  if (quoted & (Q_HERE_DOCUMENT|Q_DOUBLE_QUOTES))
	    tword->flags |= W_QUOTED;
	  list = make_word_list (tword, (WORD_LIST *)NULL);
	}
#else
      else
	list = (WORD_LIST *)NULL;
#endif
    }
  else if (word->flags & W_NOSPLIT)
    {
      tword = make_bare_word (istring);
      if (word->flags & W_ASSIGNMENT)
	tword->flags |= W_ASSIGNMENT;	/* XXX */
      if (word->flags & W_COMPASSIGN)
	tword->flags |= W_COMPASSIGN;	/* XXX */
      if (word->flags & W_NOGLOB)
	tword->flags |= W_NOGLOB;	/* XXX */
      if (word->flags & W_NOBRACE)
	tword->flags |= W_NOBRACE;	/* XXX */
      if (word->flags & W_NOEXPAND)
	tword->flags |= W_NOEXPAND;	/* XXX */
      if (quoted & (Q_HERE_DOCUMENT|Q_DOUBLE_QUOTES))
	tword->flags |= W_QUOTED;
      if (had_quoted_null && QUOTED_NULL (istring))
	tword->flags |= W_HASQUOTEDNULL;
      list = make_word_list (tword, (WORD_LIST *)NULL);
    }
  else
    {
      char *ifs_chars;
      char *tstring;

      ifs_chars = (quoted_dollar_at || has_dollar_at) ? ifs_value : (char *)NULL;

      /* If we have $@, we need to split the results no matter what.  If
	 IFS is unset or NULL, string_list_dollar_at has separated the
	 positional parameters with a space, so we split on space (we have
	 set ifs_chars to " \t\n" above if ifs is unset).  If IFS is set,
	 string_list_dollar_at has separated the positional parameters
	 with the first character of $IFS, so we split on $IFS.  If
	 SPLIT_ON_SPACES is set, we expanded $* (unquoted) with IFS either
	 unset or null, and we want to make sure that we split on spaces
	 regardless of what else has happened to IFS since the expansion,
	 or we expanded "$@" with IFS null and we need to split the positional
	 parameters into separate words. */
      if (split_on_spaces)
	list = list_string (istring, " ", 1);	/* XXX quoted == 1? */
      /* If we have $@ (has_dollar_at != 0) and we are in a context where we
	 don't want to split the result (W_NOSPLIT2), and we are not quoted,
	 we have already separated the arguments with the first character of
	 $IFS.  In this case, we want to return a list with a single word
	 with the separator possibly replaced with a space (it's what other
	 shells seem to do).
	 quoted_dollar_at is internal to this function and is set if we are
	 passed an argument that is unquoted (quoted == 0) but we encounter a
	 double-quoted $@ while expanding it. */
      else if (has_dollar_at && quoted_dollar_at == 0 && ifs_chars && quoted == 0 && (word->flags & W_NOSPLIT2))
	{
	  /* Only split and rejoin if we have to */
	  if (*ifs_chars && *ifs_chars != ' ')
	    {
	      list = list_string (istring, *ifs_chars ? ifs_chars : " ", 1);
	      tstring = string_list (list);
	    }
	  else
	    tstring = istring;
	  tword = make_bare_word (tstring);
	  if (tstring != istring)
	    free (tstring);
	  goto set_word_flags;
	}
      /* This is the attempt to make $* in an assignment context (a=$*) and
	 array variables subscripted with * in an assignment context (a=${foo[*]})
	 behave similarly.  It has side effects that, though they increase
	 compatibility with other shells, are not backwards compatible. */
#if 0
      else if (has_dollar_at && quoted == 0 && ifs_chars && (word->flags & W_ASSIGNRHS))
	{
	  tword = make_bare_word (istring);
	  goto set_word_flags;
	}
#endif
      else if (has_dollar_at && ifs_chars)
	list = list_string (istring, *ifs_chars ? ifs_chars : " ", 1);
      else
	{
	  tword = make_bare_word (istring);
set_word_flags:
	  if ((quoted & (Q_DOUBLE_QUOTES|Q_HERE_DOCUMENT)) || (quoted_state == WHOLLY_QUOTED))
	    tword->flags |= W_QUOTED;
	  if (word->flags & W_ASSIGNMENT)
	    tword->flags |= W_ASSIGNMENT;
	  if (word->flags & W_COMPASSIGN)
	    tword->flags |= W_COMPASSIGN;
	  if (word->flags & W_NOGLOB)
	    tword->flags |= W_NOGLOB;
	  if (word->flags & W_NOBRACE)
	    tword->flags |= W_NOBRACE;
	  if (word->flags & W_NOEXPAND)
	    tword->flags |= W_NOEXPAND;
	  if (had_quoted_null && QUOTED_NULL (istring))
	    tword->flags |= W_HASQUOTEDNULL;	/* XXX */
	  list = make_word_list (tword, (WORD_LIST *)NULL);
	}
    }

  free (istring);
  return (list);