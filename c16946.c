parameter_brace_expand_word (name, var_is_special, quoted, pflags, indp)
     char *name;
     int var_is_special, quoted, pflags;
     arrayind_t *indp;
{
  WORD_DESC *ret;
  char *temp, *tt;
  intmax_t arg_index;
  SHELL_VAR *var;
  int atype, rflags;
  arrayind_t ind;

  ret = 0;
  temp = 0;
  rflags = 0;

  if (indp)
    *indp = INTMAX_MIN;

  /* Handle multiple digit arguments, as in ${11}. */  
  if (legal_number (name, &arg_index))
    {
      tt = get_dollar_var_value (arg_index);
      if (tt)
 	temp = (*tt && (quoted & (Q_DOUBLE_QUOTES|Q_HERE_DOCUMENT)))
 		  ? quote_string (tt)
 		  : quote_escapes (tt);
      else
        temp = (char *)NULL;
      FREE (tt);
    }
  else if (var_is_special)      /* ${@} */
    {
      int sindex;
      tt = (char *)xmalloc (2 + strlen (name));
      tt[sindex = 0] = '$';
      strcpy (tt + 1, name);

      ret = param_expand (tt, &sindex, quoted, (int *)NULL, (int *)NULL,
			  (int *)NULL, (int *)NULL, pflags);
      free (tt);
    }
#if defined (ARRAY_VARS)
  else if (valid_array_reference (name, 0))
    {
expand_arrayref:
      if (pflags & PF_ASSIGNRHS)
	{
	  var = array_variable_part (name, &tt, (int *)0);
	  if (ALL_ELEMENT_SUB (tt[0]) && tt[1] == ']')
	    {
	      /* Only treat as double quoted if array variable */
	      if (var && (array_p (var) || assoc_p (var)))
		temp = array_value (name, quoted|Q_DOUBLE_QUOTES, 0, &atype, &ind);
	      else		
		temp = array_value (name, quoted, 0, &atype, &ind);
	    }
	  else
	    temp = array_value (name, quoted, 0, &atype, &ind);
	}
      else
	temp = array_value (name, quoted, 0, &atype, &ind);
      if (atype == 0 && temp)
	{
	  temp = (*temp && (quoted & (Q_DOUBLE_QUOTES|Q_HERE_DOCUMENT)))
		    ? quote_string (temp)
		    : quote_escapes (temp);
	  rflags |= W_ARRAYIND;
	  if (indp)
	    *indp = ind;
	} 		  
      else if (atype == 1 && temp && QUOTED_NULL (temp) && (quoted & (Q_DOUBLE_QUOTES|Q_HERE_DOCUMENT)))
	rflags |= W_HASQUOTEDNULL;
    }
#endif
  else if (var = find_variable (name))
    {
      if (var_isset (var) && invisible_p (var) == 0)
	{
#if defined (ARRAY_VARS)
	  if (assoc_p (var))
	    temp = assoc_reference (assoc_cell (var), "0");
	  else if (array_p (var))
	    temp = array_reference (array_cell (var), 0);
	  else
	    temp = value_cell (var);
#else
	  temp = value_cell (var);
#endif

	  if (temp)
	    temp = (*temp && (quoted & (Q_DOUBLE_QUOTES|Q_HERE_DOCUMENT)))
		      ? quote_string (temp)
		      : quote_escapes (temp);
	}
      else
	temp = (char *)NULL;
    }
  else if (var = find_variable_last_nameref (name, 0))
    {
      temp = nameref_cell (var);
#if defined (ARRAY_VARS)
      /* Handle expanding nameref whose value is x[n] */
      if (temp && *temp && valid_array_reference (temp, 0))
	{
	  name = temp;
	  goto expand_arrayref;
	}
      else
#endif
      /* y=2 ; typeset -n x=y; echo ${x} is not the same as echo ${2} in ksh */
      if (temp && *temp && legal_identifier (temp) == 0)
        {
	  last_command_exit_value = EXECUTION_FAILURE;
	  report_error (_("%s: invalid variable name for name reference"), temp);
	  temp = &expand_param_error;
        }
      else
	temp = (char *)NULL;
    }
  else
    temp = (char *)NULL;

  if (ret == 0)
    {
      ret = alloc_word_desc ();
      ret->word = temp;
      ret->flags |= rflags;
    }
  return ret;
}