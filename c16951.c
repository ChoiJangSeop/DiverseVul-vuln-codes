get_var_and_type (varname, value, ind, quoted, flags, varp, valp)
     char *varname, *value;
     arrayind_t ind;
     int quoted, flags;
     SHELL_VAR **varp;
     char **valp;
{
  int vtype, want_indir;
  char *temp, *vname;
  WORD_DESC *wd;
  SHELL_VAR *v;
  arrayind_t lind;

  want_indir = *varname == '!' &&
    (legal_variable_starter ((unsigned char)varname[1]) || DIGIT (varname[1])
					|| VALID_INDIR_PARAM (varname[1]));
  if (want_indir)
    vname = parameter_brace_find_indir (varname+1, SPECIAL_VAR (varname, 1), quoted, 1);
    /* XXX - what if vname == 0 || *vname == 0 ? */
  else
    vname = varname;

  if (vname == 0)
    {
      vtype = VT_VARIABLE;
      *varp = (SHELL_VAR *)NULL;
      *valp = (char *)NULL;
      return (vtype);
    }

  /* This sets vtype to VT_VARIABLE or VT_POSPARMS */
  vtype = (vname[0] == '@' || vname[0] == '*') && vname[1] == '\0';
  if (vtype == VT_POSPARMS && vname[0] == '*')
    vtype |= VT_STARSUB;
  *varp = (SHELL_VAR *)NULL;

#if defined (ARRAY_VARS)
  if (valid_array_reference (vname, 0))
    {
      v = array_variable_part (vname, &temp, (int *)0);
      /* If we want to signal array_value to use an already-computed index,
	 set LIND to that index */
      lind = (ind != INTMAX_MIN && (flags & AV_USEIND)) ? ind : 0;
      if (v && invisible_p (v))
	{
	  vtype = VT_ARRAYMEMBER;
	  *varp = (SHELL_VAR *)NULL;
	  *valp = (char *)NULL;
	}
      if (v && (array_p (v) || assoc_p (v)))
	{ /* [ */
	  if (ALL_ELEMENT_SUB (temp[0]) && temp[1] == ']')
	    {
	      /* Callers have to differentiate between indexed and associative */
	      vtype = VT_ARRAYVAR;
	      if (temp[0] == '*')
		vtype |= VT_STARSUB;
	      *valp = array_p (v) ? (char *)array_cell (v) : (char *)assoc_cell (v);
	    }
	  else
	    {
	      vtype = VT_ARRAYMEMBER;
	      *valp = array_value (vname, Q_DOUBLE_QUOTES, flags, (int *)NULL, &lind);
	    }
	  *varp = v;
	}
      else if (v && (ALL_ELEMENT_SUB (temp[0]) && temp[1] == ']'))
	{
	  vtype = VT_VARIABLE;
	  *varp = v;
	  if (quoted & (Q_DOUBLE_QUOTES|Q_HERE_DOCUMENT))
	    *valp = dequote_string (value);
	  else
	    *valp = dequote_escapes (value);
	}
      else
	{
	  vtype = VT_ARRAYMEMBER;
	  *varp = v;
	  *valp = array_value (vname, Q_DOUBLE_QUOTES, flags, (int *)NULL, &lind);
	}
    }
  else if ((v = find_variable (vname)) && (invisible_p (v) == 0) && (assoc_p (v) || array_p (v)))
    {
      vtype = VT_ARRAYMEMBER;
      *varp = v;
      *valp = assoc_p (v) ? assoc_reference (assoc_cell (v), "0") : array_reference (array_cell (v), 0);
    }
  else
#endif
    {
      if (value && vtype == VT_VARIABLE)
	{
	  *varp = find_variable (vname);
	  if (quoted & (Q_DOUBLE_QUOTES|Q_HERE_DOCUMENT))
	    *valp = dequote_string (value);
	  else
	    *valp = dequote_escapes (value);
	}
      else
	*valp = value;
    }

  if (want_indir)
    free (vname);

  return vtype;
}