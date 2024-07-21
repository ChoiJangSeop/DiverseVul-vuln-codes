param_expand (string, sindex, quoted, expanded_something,
	      contains_dollar_at, quoted_dollar_at_p, had_quoted_null_p,
	      pflags)
     char *string;
     int *sindex, quoted, *expanded_something, *contains_dollar_at;
     int *quoted_dollar_at_p, *had_quoted_null_p, pflags;
{
  char *temp, *temp1, uerror[3], *savecmd;
  int zindex, t_index, expok;
  unsigned char c;
  intmax_t number;
  SHELL_VAR *var;
  WORD_LIST *list;
  WORD_DESC *tdesc, *ret;
  int tflag;

/*itrace("param_expand: `%s' pflags = %d", string+*sindex, pflags);*/
  zindex = *sindex;
  c = string[++zindex];

  temp = (char *)NULL;
  ret = tdesc = (WORD_DESC *)NULL;
  tflag = 0;

  /* Do simple cases first. Switch on what follows '$'. */
  switch (c)
    {
    /* $0 .. $9? */
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      temp1 = dollar_vars[TODIGIT (c)];
      if (unbound_vars_is_error && temp1 == (char *)NULL)
	{
	  uerror[0] = '$';
	  uerror[1] = c;
	  uerror[2] = '\0';
	  last_command_exit_value = EXECUTION_FAILURE;
	  err_unboundvar (uerror);
	  return (interactive_shell ? &expand_wdesc_error : &expand_wdesc_fatal);
	}
      if (temp1)
	temp = (*temp1 && (quoted & (Q_HERE_DOCUMENT|Q_DOUBLE_QUOTES)))
		  ? quote_string (temp1)
		  : quote_escapes (temp1);
      else
	temp = (char *)NULL;

      break;

    /* $$ -- pid of the invoking shell. */
    case '$':
      temp = itos (dollar_dollar_pid);
      break;

    /* $# -- number of positional parameters. */
    case '#':
      temp = itos (number_of_args ());
      break;

    /* $? -- return value of the last synchronous command. */
    case '?':
      temp = itos (last_command_exit_value);
      break;

    /* $- -- flags supplied to the shell on invocation or by `set'. */
    case '-':
      temp = which_set_flags ();
      break;

      /* $! -- Pid of the last asynchronous command. */
    case '!':
      /* If no asynchronous pids have been created, expand to nothing.
	 If `set -u' has been executed, and no async processes have
	 been created, this is an expansion error. */
      if (last_asynchronous_pid == NO_PID)
	{
	  if (expanded_something)
	    *expanded_something = 0;
	  temp = (char *)NULL;
	  if (unbound_vars_is_error)
	    {
	      uerror[0] = '$';
	      uerror[1] = c;
	      uerror[2] = '\0';
	      last_command_exit_value = EXECUTION_FAILURE;
	      err_unboundvar (uerror);
	      return (interactive_shell ? &expand_wdesc_error : &expand_wdesc_fatal);
	    }
	}
      else
	temp = itos (last_asynchronous_pid);
      break;

    /* The only difference between this and $@ is when the arg is quoted. */
    case '*':		/* `$*' */
      list = list_rest_of_args ();

#if 0
      /* According to austin-group posix proposal by Geoff Clare in
	 <20090505091501.GA10097@squonk.masqnet> of 5 May 2009:

 	"The shell shall write a message to standard error and
 	 immediately exit when it tries to expand an unset parameter
 	 other than the '@' and '*' special parameters."
      */

      if (list == 0 && unbound_vars_is_error && (pflags & PF_IGNUNBOUND) == 0)
	{
	  uerror[0] = '$';
	  uerror[1] = '*';
	  uerror[2] = '\0';
	  last_command_exit_value = EXECUTION_FAILURE;
	  err_unboundvar (uerror);
	  return (interactive_shell ? &expand_wdesc_error : &expand_wdesc_fatal);
	}
#endif

      /* If there are no command-line arguments, this should just
	 disappear if there are other characters in the expansion,
	 even if it's quoted. */
      if ((quoted & (Q_HERE_DOCUMENT|Q_DOUBLE_QUOTES)) && list == 0)
	temp = (char *)NULL;
      else if (quoted & (Q_HERE_DOCUMENT|Q_DOUBLE_QUOTES|Q_PATQUOTE))
	{
	  /* If we have "$*" we want to make a string of the positional
	     parameters, separated by the first character of $IFS, and
	     quote the whole string, including the separators.  If IFS
	     is unset, the parameters are separated by ' '; if $IFS is
	     null, the parameters are concatenated. */
	  temp = (quoted & (Q_DOUBLE_QUOTES|Q_PATQUOTE)) ? string_list_dollar_star (list) : string_list (list);
	  if (temp)
	    {
	      temp1 = (quoted & Q_DOUBLE_QUOTES) ? quote_string (temp) : temp;
	      if (*temp == 0)
		tflag |= W_HASQUOTEDNULL;
	      if (temp != temp1)
		free (temp);
	      temp = temp1;
	    }
	}
      else
	{
	  /* We check whether or not we're eventually going to split $* here,
	     for example when IFS is empty and we are processing the rhs of
	     an assignment statement.  In that case, we don't separate the
	     arguments at all.  Otherwise, if the $* is not quoted it is
	     identical to $@ */
#  if defined (HANDLE_MULTIBYTE)
	  if (expand_no_split_dollar_star && ifs_firstc[0] == 0)
#  else
	  if (expand_no_split_dollar_star && ifs_firstc == 0)
#  endif
	    temp = string_list_dollar_star (list);
	  else
	    {
	      temp = string_list_dollar_at (list, quoted, 0);
	      if (quoted == 0 && (ifs_is_set == 0 || ifs_is_null))
		tflag |= W_SPLITSPACE;
	      /* If we're not quoted but we still don't want word splitting, make
		 we quote the IFS characters to protect them from splitting (e.g.,
		 when $@ is in the string as well). */
	      else if (quoted == 0 && ifs_is_set && (pflags & PF_ASSIGNRHS))
		{
		  temp1 = quote_string (temp);
		  free (temp);
		  temp = temp1;
		}
	    }

	  if (expand_no_split_dollar_star == 0 && contains_dollar_at)
	    *contains_dollar_at = 1;
	}

      dispose_words (list);
      break;

    /* When we have "$@" what we want is "$1" "$2" "$3" ... This
       means that we have to turn quoting off after we split into
       the individually quoted arguments so that the final split
       on the first character of $IFS is still done.  */
    case '@':		/* `$@' */
      list = list_rest_of_args ();

#if 0
      /* According to austin-group posix proposal by Geoff Clare in
	 <20090505091501.GA10097@squonk.masqnet> of 5 May 2009:

 	"The shell shall write a message to standard error and
 	 immediately exit when it tries to expand an unset parameter
 	 other than the '@' and '*' special parameters."
      */

      if (list == 0 && unbound_vars_is_error && (pflags & PF_IGNUNBOUND) == 0)
	{
	  uerror[0] = '$';
	  uerror[1] = '@';
	  uerror[2] = '\0';
	  last_command_exit_value = EXECUTION_FAILURE;
	  err_unboundvar (uerror);
	  return (interactive_shell ? &expand_wdesc_error : &expand_wdesc_fatal);
	}
#endif

      /* We want to flag the fact that we saw this.  We can't turn
	 off quoting entirely, because other characters in the
	 string might need it (consider "\"$@\""), but we need some
	 way to signal that the final split on the first character
	 of $IFS should be done, even though QUOTED is 1. */
      /* XXX - should this test include Q_PATQUOTE? */
      if (quoted_dollar_at_p && (quoted & (Q_HERE_DOCUMENT|Q_DOUBLE_QUOTES)))
	*quoted_dollar_at_p = 1;
      if (contains_dollar_at)
	*contains_dollar_at = 1;

      /* We want to separate the positional parameters with the first
	 character of $IFS in case $IFS is something other than a space.
	 We also want to make sure that splitting is done no matter what --
	 according to POSIX.2, this expands to a list of the positional
	 parameters no matter what IFS is set to. */
      /* XXX - what to do when in a context where word splitting is not
	 performed? Even when IFS is not the default, posix seems to imply
	 that we behave like unquoted $* ?  Maybe we should use PF_NOSPLIT2
	 here. */
      temp = string_list_dollar_at (list, (pflags & PF_ASSIGNRHS) ? (quoted|Q_DOUBLE_QUOTES) : quoted, 0);

      tflag |= W_DOLLARAT;
      dispose_words (list);
      break;

    case LBRACE:
      tdesc = parameter_brace_expand (string, &zindex, quoted, pflags,
				      quoted_dollar_at_p,
				      contains_dollar_at);

      if (tdesc == &expand_wdesc_error || tdesc == &expand_wdesc_fatal)
	return (tdesc);
      temp = tdesc ? tdesc->word : (char *)0;

      /* XXX */
      /* Quoted nulls should be removed if there is anything else
	 in the string. */
      /* Note that we saw the quoted null so we can add one back at
	 the end of this function if there are no other characters
	 in the string, discard TEMP, and go on.  The exception to
	 this is when we have "${@}" and $1 is '', since $@ needs
	 special handling. */
      if (tdesc && tdesc->word && (tdesc->flags & W_HASQUOTEDNULL) && QUOTED_NULL (temp))
	{
	  if (had_quoted_null_p)
	    *had_quoted_null_p = 1;
	  if (*quoted_dollar_at_p == 0)
	    {
	      free (temp);
	      tdesc->word = temp = (char *)NULL;
	    }
	    
	}

      ret = tdesc;
      goto return0;

    /* Do command or arithmetic substitution. */
    case LPAREN:
      /* We have to extract the contents of this paren substitution. */
      t_index = zindex + 1;
      temp = extract_command_subst (string, &t_index, 0);
      zindex = t_index;

      /* For Posix.2-style `$(( ))' arithmetic substitution,
	 extract the expression and pass it to the evaluator. */
      if (temp && *temp == LPAREN)
	{
	  char *temp2;
	  temp1 = temp + 1;
	  temp2 = savestring (temp1);
	  t_index = strlen (temp2) - 1;

	  if (temp2[t_index] != RPAREN)
	    {
	      free (temp2);
	      goto comsub;
	    }

	  /* Cut off ending `)' */
	  temp2[t_index] = '\0';

	  if (chk_arithsub (temp2, t_index) == 0)
	    {
	      free (temp2);
#if 0
	      internal_warning (_("future versions of the shell will force evaluation as an arithmetic substitution"));
#endif
	      goto comsub;
	    }

	  /* Expand variables found inside the expression. */
	  temp1 = expand_arith_string (temp2, Q_DOUBLE_QUOTES|Q_ARITH);
	  free (temp2);

arithsub:
	  /* No error messages. */
	  savecmd = this_command_name;
	  this_command_name = (char *)NULL;
	  number = evalexp (temp1, &expok);
	  this_command_name = savecmd;
	  free (temp);
	  free (temp1);
	  if (expok == 0)
	    {
	      if (interactive_shell == 0 && posixly_correct)
		{
		  last_command_exit_value = EXECUTION_FAILURE;
		  return (&expand_wdesc_fatal);
		}
	      else
		return (&expand_wdesc_error);
	    }
	  temp = itos (number);
	  break;
	}

comsub:
      if (pflags & PF_NOCOMSUB)
	/* we need zindex+1 because string[zindex] == RPAREN */
	temp1 = substring (string, *sindex, zindex+1);
      else
	{
	  tdesc = command_substitute (temp, quoted);
	  temp1 = tdesc ? tdesc->word : (char *)NULL;
	  if (tdesc)
	    dispose_word_desc (tdesc);
	}
      FREE (temp);
      temp = temp1;
      break;

    /* Do POSIX.2d9-style arithmetic substitution.  This will probably go
       away in a future bash release. */
    case '[':
      /* Extract the contents of this arithmetic substitution. */
      t_index = zindex + 1;
      temp = extract_arithmetic_subst (string, &t_index);
      zindex = t_index;
      if (temp == 0)
	{
	  temp = savestring (string);
	  if (expanded_something)
	    *expanded_something = 0;
	  goto return0;
	}	  

       /* Do initial variable expansion. */
      temp1 = expand_arith_string (temp, Q_DOUBLE_QUOTES|Q_ARITH);

      goto arithsub;

    default:
      /* Find the variable in VARIABLE_LIST. */
      temp = (char *)NULL;

      for (t_index = zindex; (c = string[zindex]) && legal_variable_char (c); zindex++)
	;
      temp1 = (zindex > t_index) ? substring (string, t_index, zindex) : (char *)NULL;

      /* If this isn't a variable name, then just output the `$'. */
      if (temp1 == 0 || *temp1 == '\0')
	{
	  FREE (temp1);
	  temp = (char *)xmalloc (2);
	  temp[0] = '$';
	  temp[1] = '\0';
	  if (expanded_something)
	    *expanded_something = 0;
	  goto return0;
	}

      /* If the variable exists, return its value cell. */
      var = find_variable (temp1);

      if (var && invisible_p (var) == 0 && var_isset (var))
	{
#if defined (ARRAY_VARS)
	  if (assoc_p (var) || array_p (var))
	    {
	      temp = array_p (var) ? array_reference (array_cell (var), 0)
				   : assoc_reference (assoc_cell (var), "0");
	      if (temp)
		temp = (*temp && (quoted & (Q_HERE_DOCUMENT|Q_DOUBLE_QUOTES)))
			  ? quote_string (temp)
			  : quote_escapes (temp);
	      else if (unbound_vars_is_error)
		goto unbound_variable;
	    }
	  else
#endif
	    {
	      temp = value_cell (var);

	      temp = (*temp && (quoted & (Q_HERE_DOCUMENT|Q_DOUBLE_QUOTES)))
			? quote_string (temp)
			: quote_escapes (temp);
	    }

	  free (temp1);

	  goto return0;
	}
      else if (var && (invisible_p (var) || var_isset (var) == 0))
	temp = (char *)NULL;
      else if ((var = find_variable_last_nameref (temp1, 0)) && var_isset (var) && invisible_p (var) == 0)
	{
	  temp = nameref_cell (var);
#if defined (ARRAY_VARS)
	  if (temp && *temp && valid_array_reference (temp, 0))
	    {
	      tdesc = parameter_brace_expand_word (temp, SPECIAL_VAR (temp, 0), quoted, pflags, (arrayind_t *)NULL);
	      if (tdesc == &expand_wdesc_error || tdesc == &expand_wdesc_fatal)
		return (tdesc);
	      ret = tdesc;
	      goto return0;
	    }
	  else
#endif
	  /* y=2 ; typeset -n x=y; echo $x is not the same as echo $2 in ksh */
	  if (temp && *temp && legal_identifier (temp) == 0)
	    {
	      last_command_exit_value = EXECUTION_FAILURE;
	      report_error (_("%s: invalid variable name for name reference"), temp);
	      return (&expand_wdesc_error);	/* XXX */
	    }
	  else
	    temp = (char *)NULL;
	}

      temp = (char *)NULL;

unbound_variable:
      if (unbound_vars_is_error)
	{
	  last_command_exit_value = EXECUTION_FAILURE;
	  err_unboundvar (temp1);
	}
      else
	{
	  free (temp1);
	  goto return0;
	}

      free (temp1);
      last_command_exit_value = EXECUTION_FAILURE;
      return ((unbound_vars_is_error && interactive_shell == 0)
		? &expand_wdesc_fatal
		: &expand_wdesc_error);
    }

  if (string[zindex])
    zindex++;

return0:
  *sindex = zindex;

  if (ret == 0)
    {
      ret = alloc_word_desc ();
      ret->flags = tflag;	/* XXX */
      ret->word = temp;
    }
  return ret;
}