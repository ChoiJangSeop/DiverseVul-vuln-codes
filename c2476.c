get_pka_info (const char *address, unsigned char *fpr)
{
#ifdef USE_ADNS
  int rc;
  adns_state state;
  const char *domain;
  char *name;
  adns_answer *answer = NULL;
  char *buffer = NULL;

  domain = strrchr (address, '@');
  if (!domain || domain == address || !domain[1])
    return NULL; /* Invalid mail address given.  */
  name = xtrymalloc (strlen (address) + 5 + 1);
  if (!name)
    return NULL;
  memcpy (name, address, domain - address);
  strcpy (stpcpy (name + (domain-address), "._pka."), domain+1);

  rc = adns_init (&state, adns_if_noerrprint, NULL);
  if (rc)
    {
      log_error ("error initializing adns: %s\n", strerror (errno));
      xfree (name);
      return NULL;
    }

  rc = adns_synchronous (state, name, adns_r_txt, adns_qf_quoteok_query,
                         &answer);
  xfree (name);
  if (rc)
    {
      log_error ("DNS query failed: %s\n", strerror (errno));
      adns_finish (state);
      return NULL;
    }
  if (answer->status != adns_s_ok
      || answer->type != adns_r_txt || !answer->nrrs)
    {
      log_error ("DNS query returned an error: %s (%s)\n",
                 adns_strerror (answer->status),
                 adns_errabbrev (answer->status));
      adns_free (answer);
      adns_finish (state);
      return NULL;
    }

  /* We use a PKA records iff there is exactly one record.  */
  if (answer->nrrs == 1 && answer->rrs.manyistr[0]->i != -1)
    {
      buffer = xtrystrdup (answer->rrs.manyistr[0]->str);
      if (parse_txt_record (buffer, fpr))
        {
          xfree (buffer);
          buffer = NULL;   /* Not a valid gpg trustdns RR. */
        }
    }

  adns_free (answer);
  adns_finish (state);
  return buffer;

#else /*!USE_ADNS*/
  unsigned char answer[PACKETSZ];
  int anslen;
  int qdcount, ancount;
  int rc;
  unsigned char *p, *pend;
  const char *domain;
  char *name;
  HEADER header;

  domain = strrchr (address, '@');
  if (!domain || domain == address || !domain[1])
    return NULL; /* invalid mail address given. */

  name = xtrymalloc (strlen (address) + 5 + 1);
  if (!name)
    return NULL;
  memcpy (name, address, domain - address);
  strcpy (stpcpy (name + (domain-address), "._pka."), domain+1);

  anslen = res_query (name, C_IN, T_TXT, answer, PACKETSZ);
  xfree (name);
  if (anslen < sizeof(HEADER))
    return NULL; /* DNS resolver returned a too short answer. */

  /* Don't despair: A good compiler should optimize this away, as
     header is just 32 byte and constant at compile time.  It's
     one way to comply with strict aliasing rules.  */
  memcpy (&header, answer, sizeof (header));

  if ( (rc=header.rcode) != NOERROR )
    return NULL; /* DNS resolver returned an error. */

  /* We assume that PACKETSZ is large enough and don't do dynmically
     expansion of the buffer. */
  if (anslen > PACKETSZ)
    return NULL; /* DNS resolver returned a too long answer */

  qdcount = ntohs (header.qdcount);
  ancount = ntohs (header.ancount);

  if (!ancount)
    return NULL; /* Got no answer. */

  p = answer + sizeof (HEADER);
  pend = answer + anslen; /* Actually points directly behind the buffer. */

  while (qdcount-- && p < pend)
    {
      rc = dn_skipname (p, pend);
      if (rc == -1)
        return NULL;
      p += rc + QFIXEDSZ;
    }

  if (ancount > 1)
    return NULL; /* more than one possible gpg trustdns record - none used. */

  while (ancount-- && p <= pend)
    {
      unsigned int type, class, txtlen, n;
      char *buffer, *bufp;

      rc = dn_skipname (p, pend);
      if (rc == -1)
        return NULL;
      p += rc;
      if (p >= pend - 10)
        return NULL; /* RR too short. */

      type = *p++ << 8;
      type |= *p++;
      class = *p++ << 8;
      class |= *p++;
      p += 4;
      txtlen = *p++ << 8;
      txtlen |= *p++;
      if (type != T_TXT || class != C_IN)
        return NULL; /* Answer does not match the query. */

      buffer = bufp = xmalloc (txtlen + 1);
      while (txtlen && p < pend)
        {
          for (n = *p++, txtlen--; txtlen && n && p < pend; txtlen--, n--)
            *bufp++ = *p++;
        }
      *bufp = 0;
      if (parse_txt_record (buffer, fpr))
        {
          xfree (buffer);
          return NULL; /* Not a valid gpg trustdns RR. */
        }
      return buffer;
    }

  return NULL;
#endif /*!USE_ADNS*/
}