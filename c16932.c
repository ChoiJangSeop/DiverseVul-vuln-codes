array_value_internal (s, quoted, flags, rtype, indp)
     char *s;
     int quoted, flags, *rtype;
     arrayind_t *indp;
{
  int len;
  arrayind_t ind;
  char *akey;
  char *retval, *t, *temp;
  WORD_LIST *l;
  SHELL_VAR *var;

  var = array_variable_part (s, &t, &len);

  /* Expand the index, even if the variable doesn't exist, in case side
     effects are needed, like ${w[i++]} where w is unset. */
#if 0
  if (var == 0)
    return (char *)NULL;
#endif

  if (len == 0)
    return ((char *)NULL);	/* error message already printed */

  /* [ */
  akey = 0;
  if (ALL_ELEMENT_SUB (t[0]) && t[1] == ']')
    {
      if (rtype)
	*rtype = (t[0] == '*') ? 1 : 2;
      if ((flags & AV_ALLOWALL) == 0)
	{
	  err_badarraysub (s);
	  return ((char *)NULL);
	}
      else if (var == 0 || value_cell (var) == 0)	/* XXX - check for invisible_p(var) ? */
	return ((char *)NULL);
      else if (array_p (var) == 0 && assoc_p (var) == 0)
	l = add_string_to_list (value_cell (var), (WORD_LIST *)NULL);
      else if (assoc_p (var))
	{
	  l = assoc_to_word_list (assoc_cell (var));
	  if (l == (WORD_LIST *)NULL)
	    return ((char *)NULL);
	}
      else
	{
	  l = array_to_word_list (array_cell (var));
	  if (l == (WORD_LIST *)NULL)
	    return ((char *) NULL);
	}

      if (t[0] == '*' && (quoted & (Q_HERE_DOCUMENT|Q_DOUBLE_QUOTES)))
	{
	  temp = string_list_dollar_star (l);
	  retval = quote_string (temp);		/* XXX - leak here */
	  free (temp);
	}
      else	/* ${name[@]} or unquoted ${name[*]} */
	retval = string_list_dollar_at (l, quoted, 0);	/* XXX - leak here */

      dispose_words (l);
    }
  else
    {
      if (rtype)
	*rtype = 0;
      if (var == 0 || array_p (var) || assoc_p (var) == 0)
	{
	  if ((flags & AV_USEIND) == 0 || indp == 0)
	    {
	      ind = array_expand_index (var, t, len);
	      if (ind < 0)
		{
		  /* negative subscripts to indexed arrays count back from end */
		  if (var && array_p (var))
		    ind = array_max_index (array_cell (var)) + 1 + ind;
		  if (ind < 0)
		    INDEX_ERROR();
		}
	      if (indp)
		*indp = ind;
	    }
	  else if (indp)
	    ind = *indp;
	}
      else if (assoc_p (var))
	{
	  t[len - 1] = '\0';
	  akey = expand_assignment_string_to_string (t, 0);	/* [ */
	  t[len - 1] = ']';
	  if (akey == 0 || *akey == 0)
	    {
	      FREE (akey);
	      INDEX_ERROR();
	    }
	}
     
      if (var == 0 || value_cell (var) == 0)	/* XXX - check invisible_p(var) ? */
	{
          FREE (akey);
	  return ((char *)NULL);
	}
      if (array_p (var) == 0 && assoc_p (var) == 0)
	return (ind == 0 ? value_cell (var) : (char *)NULL);
      else if (assoc_p (var))
        {
	  retval = assoc_reference (assoc_cell (var), akey);
	  free (akey);
        }
      else
	retval = array_reference (array_cell (var), ind);
    }

  return retval;
}