parameter_brace_expand_length (name)
     char *name;
{
  char *t, *newname;
  intmax_t number, arg_index;
  WORD_LIST *list;
#if defined (ARRAY_VARS)
  SHELL_VAR *var;
#endif

  if (name[1] == '\0')			/* ${#} */
    number = number_of_args ();
  else if ((name[1] == '@' || name[1] == '*') && name[2] == '\0')	/* ${#@}, ${#*} */
    number = number_of_args ();
  else if ((sh_syntaxtab[(unsigned char) name[1]] & CSPECVAR) && name[2] == '\0')
    {
      /* Take the lengths of some of the shell's special parameters. */
      switch (name[1])
	{
	case '-':
	  t = which_set_flags ();
	  break;
	case '?':
	  t = itos (last_command_exit_value);
	  break;
	case '$':
	  t = itos (dollar_dollar_pid);
	  break;
	case '!':
	  if (last_asynchronous_pid == NO_PID)
	    t = (char *)NULL;	/* XXX - error if set -u set? */
	  else
	    t = itos (last_asynchronous_pid);
	  break;
	case '#':
	  t = itos (number_of_args ());
	  break;
	}
      number = STRLEN (t);
      FREE (t);
    }
#if defined (ARRAY_VARS)
  else if (valid_array_reference (name + 1, 0))
    number = array_length_reference (name + 1);
#endif /* ARRAY_VARS */
  else
    {
      number = 0;

      if (legal_number (name + 1, &arg_index))		/* ${#1} */
	{
	  t = get_dollar_var_value (arg_index);
	  if (t == 0 && unbound_vars_is_error)
	    return INTMAX_MIN;
	  number = MB_STRLEN (t);
	  FREE (t);
	}
#if defined (ARRAY_VARS)
      else if ((var = find_variable (name + 1)) && (invisible_p (var) == 0) && (array_p (var) || assoc_p (var)))
	{
	  if (assoc_p (var))
	    t = assoc_reference (assoc_cell (var), "0");
	  else
	    t = array_reference (array_cell (var), 0);
	  if (t == 0 && unbound_vars_is_error)
	    return INTMAX_MIN;
	  number = MB_STRLEN (t);
	}
#endif
      else				/* ${#PS1} */
	{
	  newname = savestring (name);
	  newname[0] = '$';
	  list = expand_string (newname, Q_DOUBLE_QUOTES);
	  t = list ? string_list (list) : (char *)NULL;
	  free (newname);
	  if (list)
	    dispose_words (list);

	  number = t ? MB_STRLEN (t) : 0;
	  FREE (t);
	}
    }

  return (number);
}