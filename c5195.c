asn1_find_node (asn1_node pointer, const char *name)
{
  asn1_node p;
  char *n_end, n[ASN1_MAX_NAME_SIZE + 1];
  const char *n_start;
  unsigned int nsize;
  unsigned int nhash;

  if (pointer == NULL)
    return NULL;

  if (name == NULL)
    return NULL;

  p = pointer;
  n_start = name;

  if (name[0] == '?' && name[1] == 'C' && p->name[0] == '?')
    { /* ?CURRENT */
      n_start = strchr(n_start, '.');
      if (n_start)
        n_start++;
    }
  else if (p->name[0] != 0)
    {				/* has *pointer got a name ? */
      n_end = strchr (n_start, '.');	/* search the first dot */
      if (n_end)
	{
	  nsize = n_end - n_start;
	  memcpy (n, n_start, nsize);
	  n[nsize] = 0;
	  n_start = n_end;
	  n_start++;

	  nhash = hash_pjw_bare (n, nsize);
	}
      else
	{
	  nsize = _asn1_str_cpy (n, sizeof (n), n_start);
	  nhash = hash_pjw_bare (n, nsize);

	  n_start = NULL;
	}

      while (p)
	{
	  if (nhash == p->name_hash && (!strcmp (p->name, n)))
	    break;
	  else
	    p = p->right;
	}			/* while */

      if (p == NULL)
	return NULL;
    }
  else
    {				/* *pointer doesn't have a name */
      if (n_start[0] == 0)
	return p;
    }

  while (n_start)
    {				/* Has the end of NAME been reached? */
      n_end = strchr (n_start, '.');	/* search the next dot */
      if (n_end)
	{
	  nsize = n_end - n_start;
	  memcpy (n, n_start, nsize);
	  n[nsize] = 0;
	  n_start = n_end;
	  n_start++;

	  nhash = hash_pjw_bare (n, nsize);
	}
      else
	{
	  nsize = _asn1_str_cpy (n, sizeof (n), n_start);
	  nhash = hash_pjw_bare (n, nsize);
	  n_start = NULL;
	}

      if (p->down == NULL)
	return NULL;

      p = p->down;
      if (p == NULL)
        return NULL;

      /* The identifier "?LAST" indicates the last element
         in the right chain. */
      if (n[0] == '?' && n[1] == 'L') /* ?LAST */
	{
	  while (p->right)
	    p = p->right;
	}
      else
	{			/* no "?LAST" */
	  while (p)
	    {
	      if (p->name_hash == nhash && !strcmp (p->name, n))
		break;
	      else
		p = p->right;
	    }
	}
      if (p == NULL)
        return NULL;
    }				/* while */

  return p;
}