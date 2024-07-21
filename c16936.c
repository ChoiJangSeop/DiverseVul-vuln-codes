parameter_brace_expand (string, indexp, quoted, pflags, quoted_dollar_atp, contains_dollar_at)
     char *string;
     int *indexp, quoted, pflags, *quoted_dollar_atp, *contains_dollar_at;
{
  int check_nullness, var_is_set, var_is_null, var_is_special;
  int want_substring, want_indir, want_patsub, want_casemod;
  char *name, *value, *temp, *temp1;
  WORD_DESC *tdesc, *ret;
  int t_index, sindex, c, tflag, modspec;
  intmax_t number;
  arrayind_t ind;

  temp = temp1 = value = (char *)NULL;
  var_is_set = var_is_null = var_is_special = check_nullness = 0;
  want_substring = want_indir = want_patsub = want_casemod = 0;

  sindex = *indexp;
  t_index = ++sindex;
  /* ${#var} doesn't have any of the other parameter expansions on it. */
  if (string[t_index] == '#' && legal_variable_starter (string[t_index+1]))		/* {{ */
    name = string_extract (string, &t_index, "}", SX_VARNAME);
  else
#if defined (CASEMOD_EXPANSIONS)
    /* To enable case-toggling expansions using the `~' operator character
       change the 1 to 0. */
#  if defined (CASEMOD_CAPCASE)
    name = string_extract (string, &t_index, "#%^,~:-=?+/@}", SX_VARNAME);
#  else
    name = string_extract (string, &t_index, "#%^,:-=?+/@}", SX_VARNAME);
#  endif /* CASEMOD_CAPCASE */
#else
    name = string_extract (string, &t_index, "#%:-=?+/@}", SX_VARNAME);
#endif /* CASEMOD_EXPANSIONS */

  /* Handle ${@[stuff]} now that @ is a word expansion operator.  Not exactly
     the cleanest code ever. */
  if (*name == 0 && sindex == t_index && string[sindex] == '@')
    {
      name = (char *)xrealloc (name, 2);
      name[0] = '@';
      name[1] = '\0';
      t_index++;
    }
  else if (*name == '!' && t_index > sindex && string[t_index] == '@' && string[t_index+1] == '}')
    {
      name = (char *)xrealloc (name, t_index - sindex + 2);
      name[t_index - sindex] = '@';
      name[t_index - sindex + 1] = '\0';
      t_index++;
    }

  ret = 0;
  tflag = 0;

  ind = INTMAX_MIN;

  /* If the name really consists of a special variable, then make sure
     that we have the entire name.  We don't allow indirect references
     to special variables except `#', `?', `@' and `*'.  This clause is
     designed to handle ${#SPECIAL} and ${!SPECIAL}, not anything more
     general. */
  if ((sindex == t_index && VALID_SPECIAL_LENGTH_PARAM (string[t_index])) ||
      (sindex == t_index && string[sindex] == '#' && VALID_SPECIAL_LENGTH_PARAM (string[sindex + 1])) ||
      (sindex == t_index - 1 && string[sindex] == '!' && VALID_INDIR_PARAM (string[t_index])))
    {
      t_index++;
      temp1 = string_extract (string, &t_index, "#%:-=?+/@}", 0);
      name = (char *)xrealloc (name, 3 + (strlen (temp1)));
      *name = string[sindex];
      if (string[sindex] == '!')
	{
	  /* indirect reference of $#, $?, $@, or $* */
	  name[1] = string[sindex + 1];
	  strcpy (name + 2, temp1);
	}
      else	
	strcpy (name + 1, temp1);
      free (temp1);
    }
  sindex = t_index;

  /* Find out what character ended the variable name.  Then
     do the appropriate thing. */
  if (c = string[sindex])
    sindex++;

  /* If c is followed by one of the valid parameter expansion
     characters, move past it as normal.  If not, assume that
     a substring specification is being given, and do not move
     past it. */
  if (c == ':' && VALID_PARAM_EXPAND_CHAR (string[sindex]))
    {
      check_nullness++;
      if (c = string[sindex])
	sindex++;
    }
  else if (c == ':' && string[sindex] != RBRACE)
    want_substring = 1;
  else if (c == '/' /* && string[sindex] != RBRACE */)	/* XXX */
    want_patsub = 1;
#if defined (CASEMOD_EXPANSIONS)
  else if (c == '^' || c == ',' || c == '~')
    {
      modspec = c;
      want_casemod = 1;
    }
#endif

  /* Catch the valid and invalid brace expressions that made it through the
     tests above. */
  /* ${#-} is a valid expansion and means to take the length of $-.
     Similarly for ${#?} and ${##}... */
  if (name[0] == '#' && name[1] == '\0' && check_nullness == 0 &&
	VALID_SPECIAL_LENGTH_PARAM (c) && string[sindex] == RBRACE)
    {
      name = (char *)xrealloc (name, 3);
      name[1] = c;
      name[2] = '\0';
      c = string[sindex++];
    }

  /* ...but ${#%}, ${#:}, ${#=}, ${#+}, and ${#/} are errors. */
  if (name[0] == '#' && name[1] == '\0' && check_nullness == 0 &&
	member (c, "%:=+/") && string[sindex] == RBRACE)
    {
      temp = (char *)NULL;
      goto bad_substitution;	/* XXX - substitution error */
    }

  /* Indirect expansion begins with a `!'.  A valid indirect expansion is
     either a variable name, one of the positional parameters or a special
     variable that expands to one of the positional parameters. */
  want_indir = *name == '!' &&
    (legal_variable_starter ((unsigned char)name[1]) || DIGIT (name[1])
					|| VALID_INDIR_PARAM (name[1]));

  /* Determine the value of this variable. */

  /* Check for special variables, directly referenced. */
  if (SPECIAL_VAR (name, want_indir))
    var_is_special++;

  /* Check for special expansion things, like the length of a parameter */
  if (*name == '#' && name[1])
    {
      /* If we are not pointing at the character just after the
	 closing brace, then we haven't gotten all of the name.
	 Since it begins with a special character, this is a bad
	 substitution.  Also check NAME for validity before trying
	 to go on. */
      if (string[sindex - 1] != RBRACE || (valid_length_expression (name) == 0))
	{
	  temp = (char *)NULL;
	  goto bad_substitution;	/* substitution error */
	}

      number = parameter_brace_expand_length (name);
      if (number == INTMAX_MIN && unbound_vars_is_error)
	{
	  last_command_exit_value = EXECUTION_FAILURE;
	  err_unboundvar (name+1);
	  free (name);
	  return (interactive_shell ? &expand_wdesc_error : &expand_wdesc_fatal);
	}
      free (name);

      *indexp = sindex;
      if (number < 0)
        return (&expand_wdesc_error);
      else
	{
	  ret = alloc_word_desc ();
	  ret->word = itos (number);
	  return ret;
	}
    }

  /* ${@} is identical to $@. */
  if (name[0] == '@' && name[1] == '\0')
    {
      if ((quoted & (Q_HERE_DOCUMENT|Q_DOUBLE_QUOTES)) && quoted_dollar_atp)
	*quoted_dollar_atp = 1;

      if (contains_dollar_at)
	*contains_dollar_at = 1;

      tflag |= W_DOLLARAT;
    }

  /* Process ${!PREFIX*} expansion. */
  if (want_indir && string[sindex - 1] == RBRACE &&
      (string[sindex - 2] == '*' || string[sindex - 2] == '@') &&
      legal_variable_starter ((unsigned char) name[1]))
    {
      char **x;
      WORD_LIST *xlist;

      temp1 = savestring (name + 1);
      number = strlen (temp1);
      temp1[number - 1] = '\0';
      x = all_variables_matching_prefix (temp1);
      xlist = strvec_to_word_list (x, 0, 0);
      if (string[sindex - 2] == '*')
	temp = string_list_dollar_star (xlist);
      else
	{
	  temp = string_list_dollar_at (xlist, quoted, 0);
	  if ((quoted & (Q_HERE_DOCUMENT|Q_DOUBLE_QUOTES)) && quoted_dollar_atp)
	    *quoted_dollar_atp = 1;
	  if (contains_dollar_at)
	    *contains_dollar_at = 1;

	  tflag |= W_DOLLARAT;
	}
      free (x);
      dispose_words (xlist);
      free (temp1);
      *indexp = sindex;

      free (name);

      ret = alloc_word_desc ();
      ret->word = temp;
      ret->flags = tflag;	/* XXX */
      return ret;
    }

#if defined (ARRAY_VARS)      
  /* Process ${!ARRAY[@]} and ${!ARRAY[*]} expansion. */ /* [ */
  if (want_indir && string[sindex - 1] == RBRACE &&
      string[sindex - 2] == ']' && valid_array_reference (name+1, 0))
    {
      char *x, *x1;

      temp1 = savestring (name + 1);
      x = array_variable_name (temp1, &x1, (int *)0);	/* [ */
      FREE (x);
      if (ALL_ELEMENT_SUB (x1[0]) && x1[1] == ']')
	{
	  temp = array_keys (temp1, quoted);	/* handles assoc vars too */
	  if (x1[0] == '@')
	    {
	      if ((quoted & (Q_HERE_DOCUMENT|Q_DOUBLE_QUOTES)) && quoted_dollar_atp)
		*quoted_dollar_atp = 1;
	      if (contains_dollar_at)
		*contains_dollar_at = 1;

	      tflag |= W_DOLLARAT;
	    }	    

	  free (name);
	  free (temp1);
	  *indexp = sindex;

	  ret = alloc_word_desc ();
	  ret->word = temp;
	  ret->flags = tflag;	/* XXX */
	  return ret;
	}

      free (temp1);
    }
#endif /* ARRAY_VARS */
      
  /* Make sure that NAME is valid before trying to go on. */
  if (valid_brace_expansion_word (want_indir ? name + 1 : name,
					var_is_special) == 0)
    {
      temp = (char *)NULL;
      goto bad_substitution;		/* substitution error */
    }

  if (want_indir)
    {
      tdesc = parameter_brace_expand_indir (name + 1, var_is_special, quoted, quoted_dollar_atp, contains_dollar_at);
      if (tdesc == &expand_wdesc_error || tdesc == &expand_wdesc_fatal)
	{
	  temp = (char *)NULL;
	  goto bad_substitution;
	}
      /* Turn off the W_ARRAYIND flag because there is no way for this function
	 to return the index we're supposed to be using. */
      if (tdesc && tdesc->flags)
	tdesc->flags &= ~W_ARRAYIND;
    }
  else
    tdesc = parameter_brace_expand_word (name, var_is_special, quoted, PF_IGNUNBOUND|(pflags&(PF_NOSPLIT2|PF_ASSIGNRHS)), &ind);

  if (tdesc)
    {
      temp = tdesc->word;
      tflag = tdesc->flags;
      dispose_word_desc (tdesc);
    }
  else
    temp = (char  *)0;

  if (temp == &expand_param_error || temp == &expand_param_fatal)
    {
      FREE (name);
      FREE (value);
      return (temp == &expand_param_error ? &expand_wdesc_error : &expand_wdesc_fatal);
    }

#if defined (ARRAY_VARS)
  if (valid_array_reference (name, 0))
    {
      int qflags;

      qflags = quoted;
      /* If in a context where word splitting will not take place, treat as
	 if double-quoted.  Has effects with $* and ${array[*]} */
      if (pflags & PF_ASSIGNRHS)
	qflags |= Q_DOUBLE_QUOTES;
      chk_atstar (name, qflags, quoted_dollar_atp, contains_dollar_at);
    }
#endif

  var_is_set = temp != (char *)0;
  var_is_null = check_nullness && (var_is_set == 0 || *temp == 0);
  /* XXX - this may not need to be restricted to special variables */
  if (check_nullness)
    var_is_null |= var_is_set && var_is_special && (quoted & (Q_HERE_DOCUMENT|Q_DOUBLE_QUOTES)) && QUOTED_NULL (temp);

  /* Get the rest of the stuff inside the braces. */
  if (c && c != RBRACE)
    {
      /* Extract the contents of the ${ ... } expansion
	 according to the Posix.2 rules. */
      value = extract_dollar_brace_string (string, &sindex, quoted, (c == '%' || c == '#' || c =='/' || c == '^' || c == ',' || c ==':') ? SX_POSIXEXP|SX_WORD : SX_WORD);
      if (string[sindex] == RBRACE)
	sindex++;
      else
	goto bad_substitution;		/* substitution error */
    }
  else
    value = (char *)NULL;

  *indexp = sindex;

  /* All the cases where an expansion can possibly generate an unbound
     variable error. */
  if (want_substring || want_patsub || want_casemod || c == '#' || c == '%' || c == RBRACE)
    {
      if (var_is_set == 0 && unbound_vars_is_error && ((name[0] != '@' && name[0] != '*') || name[1]))
	{
	  last_command_exit_value = EXECUTION_FAILURE;
	  err_unboundvar (name);
	  FREE (value);
	  FREE (temp);
	  free (name);
	  return (interactive_shell ? &expand_wdesc_error : &expand_wdesc_fatal);
	}
    }
    
  /* If this is a substring spec, process it and add the result. */
  if (want_substring)
    {
      temp1 = parameter_brace_substring (name, temp, ind, value, quoted, (tflag & W_ARRAYIND) ? AV_USEIND : 0);
      FREE (name);
      FREE (value);
      FREE (temp);

      if (temp1 == &expand_param_error)
	return (&expand_wdesc_error);
      else if (temp1 == &expand_param_fatal)
	return (&expand_wdesc_fatal);

      ret = alloc_word_desc ();
      ret->word = temp1;
      /* We test quoted_dollar_atp because we want variants with double-quoted
	 "$@" to take a different code path. In fact, we make sure at the end
	 of expand_word_internal that we're only looking at these flags if
	 quoted_dollar_at == 0. */
      if (temp1 && 
          (quoted_dollar_atp == 0 || *quoted_dollar_atp == 0) &&
	  QUOTED_NULL (temp1) && (quoted & (Q_HERE_DOCUMENT|Q_DOUBLE_QUOTES)))
	ret->flags |= W_QUOTED|W_HASQUOTEDNULL;
      return ret;
    }
  else if (want_patsub)
    {
      temp1 = parameter_brace_patsub (name, temp, ind, value, quoted, pflags, (tflag & W_ARRAYIND) ? AV_USEIND : 0);
      FREE (name);
      FREE (value);
      FREE (temp);

      if (temp1 == &expand_param_error)
	return (&expand_wdesc_error);
      else if (temp1 == &expand_param_fatal)
	return (&expand_wdesc_fatal);

      ret = alloc_word_desc ();
      ret->word = temp1;
      if (temp1 && 
          (quoted_dollar_atp == 0 || *quoted_dollar_atp == 0) &&
	  QUOTED_NULL (temp1) && (quoted & (Q_HERE_DOCUMENT|Q_DOUBLE_QUOTES)))
	ret->flags |= W_QUOTED|W_HASQUOTEDNULL;
      return ret;
    }
#if defined (CASEMOD_EXPANSIONS)
  else if (want_casemod)
    {
      temp1 = parameter_brace_casemod (name, temp, ind, modspec, value, quoted, (tflag & W_ARRAYIND) ? AV_USEIND : 0);
      FREE (name);
      FREE (value);
      FREE (temp);

      if (temp1 == &expand_param_error)
	return (&expand_wdesc_error);
      else if (temp1 == &expand_param_fatal)
	return (&expand_wdesc_fatal);

      ret = alloc_word_desc ();
      ret->word = temp1;
      if (temp1 &&
          (quoted_dollar_atp == 0 || *quoted_dollar_atp == 0) &&
	  QUOTED_NULL (temp1) && (quoted & (Q_HERE_DOCUMENT|Q_DOUBLE_QUOTES)))
	ret->flags |= W_QUOTED|W_HASQUOTEDNULL;
      return ret;
    }
#endif

  /* Do the right thing based on which character ended the variable name. */
  switch (c)
    {
    default:
    case '\0':
bad_substitution:
      last_command_exit_value = EXECUTION_FAILURE;
      report_error (_("%s: bad substitution"), string ? string : "??");
      FREE (value);
      FREE (temp);
      free (name);
      if (shell_compatibility_level <= 43)
	return &expand_wdesc_error;
      else
	return ((posixly_correct && interactive_shell == 0) ? &expand_wdesc_fatal : &expand_wdesc_error);

    case RBRACE:
      break;

    case '@':
      temp1 = parameter_brace_transform (name, temp, ind, value, c, quoted, (tflag & W_ARRAYIND) ? AV_USEIND : 0);
      free (temp);
      free (value);
      free (name);
      if (temp1 == &expand_param_error || temp1 == &expand_param_fatal)
	{
	  last_command_exit_value = EXECUTION_FAILURE;
	  report_error (_("%s: bad substitution"), string ? string : "??");
	  return (temp1 == &expand_param_error ? &expand_wdesc_error : &expand_wdesc_fatal);
	}

      ret = alloc_word_desc ();
      ret->word = temp1;
      if (temp1 && QUOTED_NULL (temp1) && (quoted & (Q_HERE_DOCUMENT|Q_DOUBLE_QUOTES)))
	ret->flags |= W_QUOTED|W_HASQUOTEDNULL;
      return ret;

    case '#':	/* ${param#[#]pattern} */
    case '%':	/* ${param%[%]pattern} */
      if (value == 0 || *value == '\0' || temp == 0 || *temp == '\0')
	{
	  FREE (value);
	  break;
	}
      temp1 = parameter_brace_remove_pattern (name, temp, ind, value, c, quoted, (tflag & W_ARRAYIND) ? AV_USEIND : 0);
      free (temp);
      free (value);
      free (name);

      ret = alloc_word_desc ();
      ret->word = temp1;
      if (temp1 && QUOTED_NULL (temp1) && (quoted & (Q_HERE_DOCUMENT|Q_DOUBLE_QUOTES)))
	ret->flags |= W_QUOTED|W_HASQUOTEDNULL;
      return ret;

    case '-':
    case '=':
    case '?':
    case '+':
      if (var_is_set && var_is_null == 0)
	{
	  /* If the operator is `+', we don't want the value of the named
	     variable for anything, just the value of the right hand side. */
	  if (c == '+')
	    {
	      /* XXX -- if we're double-quoted and the named variable is "$@",
			we want to turn off any special handling of "$@" --
			we're not using it, so whatever is on the rhs applies. */
	      if ((quoted & (Q_HERE_DOCUMENT|Q_DOUBLE_QUOTES)) && quoted_dollar_atp)
		*quoted_dollar_atp = 0;
	      if (contains_dollar_at)
		*contains_dollar_at = 0;

	      FREE (temp);
	      if (value)
		{
		  /* From Posix discussion on austin-group list.  Issue 221
		     requires that backslashes escaping `}' inside
		     double-quoted ${...} be removed. */
		  if (quoted & (Q_HERE_DOCUMENT|Q_DOUBLE_QUOTES))
		    quoted |= Q_DOLBRACE;
		  ret = parameter_brace_expand_rhs (name, value, c,
						    quoted,
						    pflags,
						    quoted_dollar_atp,
						    contains_dollar_at);
		  /* XXX - fix up later, esp. noting presence of
			   W_HASQUOTEDNULL in ret->flags */
		  free (value);
		}
	      else
		temp = (char *)NULL;
	    }
	  else
	    {
	      FREE (value);
	    }
	  /* Otherwise do nothing; just use the value in TEMP. */
	}
      else	/* VAR not set or VAR is NULL. */
	{
	  FREE (temp);
	  temp = (char *)NULL;
	  if (c == '=' && var_is_special)
	    {
	      last_command_exit_value = EXECUTION_FAILURE;
	      report_error (_("$%s: cannot assign in this way"), name);
	      free (name);
	      free (value);
	      return &expand_wdesc_error;
	    }
	  else if (c == '?')
	    {
	      parameter_brace_expand_error (name, value);
	      return (interactive_shell ? &expand_wdesc_error : &expand_wdesc_fatal);
	    }
	  else if (c != '+')
	    {
	      /* XXX -- if we're double-quoted and the named variable is "$@",
			we want to turn off any special handling of "$@" --
			we're not using it, so whatever is on the rhs applies. */
	      if ((quoted & (Q_HERE_DOCUMENT|Q_DOUBLE_QUOTES)) && quoted_dollar_atp)
		*quoted_dollar_atp = 0;
	      if (contains_dollar_at)
		*contains_dollar_at = 0;

	      /* From Posix discussion on austin-group list.  Issue 221 requires
		 that backslashes escaping `}' inside double-quoted ${...} be
		 removed. */
	      if (quoted & (Q_HERE_DOCUMENT|Q_DOUBLE_QUOTES))
		quoted |= Q_DOLBRACE;
	      ret = parameter_brace_expand_rhs (name, value, c, quoted, pflags,
						quoted_dollar_atp,
						contains_dollar_at);
	      /* XXX - fix up later, esp. noting presence of
		       W_HASQUOTEDNULL in tdesc->flags */
	    }
	  free (value);
	}

      break;
    }
  free (name);

  if (ret == 0)
    {
      ret = alloc_word_desc ();
      ret->flags = tflag;
      ret->word = temp;
    }
  return (ret);
}