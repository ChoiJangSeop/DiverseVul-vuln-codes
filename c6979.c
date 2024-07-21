do_parse_uri (parsed_uri_t uri, int only_local_part,
              int no_scheme_check, int force_tls)
{
  uri_tuple_t *tail;
  char *p, *p2, *p3, *pp;
  int n;

  p = uri->buffer;
  n = strlen (uri->buffer);

  /* Initialize all fields to an empty string or an empty list. */
  uri->scheme = uri->host = uri->path = p + n;
  uri->port = 0;
  uri->params = uri->query = NULL;
  uri->use_tls = 0;
  uri->is_http = 0;
  uri->opaque = 0;
  uri->v6lit = 0;
  uri->onion = 0;
  uri->explicit_port = 0;

  /* A quick validity check. */
  if (strspn (p, VALID_URI_CHARS) != n)
    return GPG_ERR_BAD_URI;	/* Invalid characters found. */

  if (!only_local_part)
    {
      /* Find the scheme. */
      if (!(p2 = strchr (p, ':')) || p2 == p)
	return GPG_ERR_BAD_URI; /* No scheme. */
      *p2++ = 0;
      for (pp=p; *pp; pp++)
       *pp = tolower (*(unsigned char*)pp);
      uri->scheme = p;
      if (!strcmp (uri->scheme, "http") && !force_tls)
        {
          uri->port = 80;
          uri->is_http = 1;
        }
      else if (!strcmp (uri->scheme, "hkp") && !force_tls)
        {
          uri->port = 11371;
          uri->is_http = 1;
        }
#ifdef USE_TLS
      else if (!strcmp (uri->scheme, "https") || !strcmp (uri->scheme,"hkps")
               || (force_tls && (!strcmp (uri->scheme, "http")
                                 || !strcmp (uri->scheme,"hkp"))))
        {
          uri->port = 443;
          uri->is_http = 1;
          uri->use_tls = 1;
        }
#endif /*USE_TLS*/
      else if (!no_scheme_check)
	return GPG_ERR_INV_URI; /* Unsupported scheme */

      p = p2;

      if (*p == '/' && p[1] == '/' ) /* There seems to be a hostname. */
	{
          p += 2;
	  if ((p2 = strchr (p, '/')))
	    *p2++ = 0;

          /* Check for username/password encoding */
          if ((p3 = strchr (p, '@')))
            {
              uri->auth = p;
              *p3++ = '\0';
              p = p3;
            }

          for (pp=p; *pp; pp++)
            *pp = tolower (*(unsigned char*)pp);

	  /* Handle an IPv6 literal */
	  if( *p == '[' && (p3=strchr( p, ']' )) )
	    {
	      *p3++ = '\0';
	      /* worst case, uri->host should have length 0, points to \0 */
	      uri->host = p + 1;
              uri->v6lit = 1;
	      p = p3;
	    }
	  else
	    uri->host = p;

	  if ((p3 = strchr (p, ':')))
	    {
	      *p3++ = '\0';
	      uri->port = atoi (p3);
              uri->explicit_port = 1;
	    }

	  if ((n = remove_escapes (uri->host)) < 0)
	    return GPG_ERR_BAD_URI;
	  if (n != strlen (uri->host))
	    return GPG_ERR_BAD_URI;	/* Hostname includes a Nul. */
	  p = p2 ? p2 : NULL;
	}
      else if (uri->is_http)
	return GPG_ERR_INV_URI; /* No Leading double slash for HTTP.  */
      else
        {
          uri->opaque = 1;
          uri->path = p;
          if (is_onion_address (uri->path))
            uri->onion = 1;
          return 0;
        }

    } /* End global URI part. */

  /* Parse the pathname part if any.  */
  if (p && *p)
    {
      /* TODO: Here we have to check params. */

      /* Do we have a query part? */
      if ((p2 = strchr (p, '?')))
        *p2++ = 0;

      uri->path = p;
      if ((n = remove_escapes (p)) < 0)
        return GPG_ERR_BAD_URI;
      if (n != strlen (p))
        return GPG_ERR_BAD_URI;	/* Path includes a Nul. */
      p = p2 ? p2 : NULL;

      /* Parse a query string if any.  */
      if (p && *p)
        {
          tail = &uri->query;
          for (;;)
            {
              uri_tuple_t elem;

              if ((p2 = strchr (p, '&')))
                *p2++ = 0;
              if (!(elem = parse_tuple (p)))
                return GPG_ERR_BAD_URI;
              *tail = elem;
              tail = &elem->next;

              if (!p2)
                break; /* Ready. */
              p = p2;
            }
        }
    }

  if (is_onion_address (uri->host))
    uri->onion = 1;

  return 0;
}