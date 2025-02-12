get_dns_cert (const char *name, estream_t *r_key,
              unsigned char **r_fpr, size_t *r_fprlen, char **r_url)
{
#ifdef USE_DNS_CERT
#ifdef USE_ADNS
  gpg_error_t err;
  adns_state state;
  adns_answer *answer = NULL;
  unsigned int ctype;
  int count;

  *r_key = NULL;
  *r_fpr = NULL;
  *r_fprlen = 0;
  *r_url = NULL;

  if (adns_init (&state, adns_if_noerrprint, NULL))
    {
      err = gpg_err_make (default_errsource, gpg_err_code_from_syserror ());
      log_error ("error initializing adns: %s\n", strerror (errno));
      return err;
    }

  if (adns_synchronous (state, name, (adns_r_unknown | my_adns_r_cert),
                        adns_qf_quoteok_query, &answer))
    {
      err = gpg_err_make (default_errsource, gpg_err_code_from_syserror ());
      /* log_error ("DNS query failed: %s\n", strerror (errno)); */
      adns_finish (state);
      return err;
    }
  if (answer->status != adns_s_ok)
    {
      /* log_error ("DNS query returned an error: %s (%s)\n", */
      /*            adns_strerror (answer->status), */
      /*            adns_errabbrev (answer->status)); */
      err = gpg_err_make (default_errsource, GPG_ERR_NOT_FOUND);
      goto leave;
    }

  err = gpg_err_make (default_errsource, GPG_ERR_NOT_FOUND);
  for (count = 0; count < answer->nrrs; count++)
    {
      int datalen = answer->rrs.byteblock[count].len;
      const unsigned char *data = answer->rrs.byteblock[count].data;

      if (datalen < 5)
        continue;  /* Truncated CERT record - skip.  */

      ctype = ((data[0] << 8) | data[1]);
      /* (key tag and algorithm fields are not required.) */
      data += 5;
      datalen -= 5;

      if (ctype == CERTTYPE_PGP && datalen >= 11)
        {
          /* CERT type is PGP.  Gpg checks for a minimum length of 11,
             thus we do the same.  */
          *r_key = es_fopenmem_init (0, "rwb", data, datalen);
          if (!*r_key)
            err = gpg_err_make (default_errsource,
                                gpg_err_code_from_syserror ());
          else
            err = 0;
          goto leave;
        }
      else if (ctype == CERTTYPE_IPGP && datalen && datalen < 1023
               && datalen >= data[0] + 1 && r_fpr && r_fprlen && r_url)
        {
          /* CERT type is IPGP.  We made sure that the data is
             plausible and that the caller requested this
             information.  */
          *r_fprlen = data[0];
          if (*r_fprlen)
            {
              *r_fpr = xtrymalloc (*r_fprlen);
              if (!*r_fpr)
                {
                  err = gpg_err_make (default_errsource,
                                      gpg_err_code_from_syserror ());
                  goto leave;
                }
              memcpy (*r_fpr, data + 1, *r_fprlen);
            }
          else
            *r_fpr = NULL;

          if (datalen > *r_fprlen + 1)
            {
              *r_url = xtrymalloc (datalen - (*r_fprlen + 1) + 1);
              if (!*r_url)
                {
                  err = gpg_err_make (default_errsource,
                                      gpg_err_code_from_syserror ());
                  xfree (*r_fpr);
                  *r_fpr = NULL;
                  goto leave;
                }
              memcpy (*r_url,
                      data + (*r_fprlen + 1), datalen - (*r_fprlen + 1));
              (*r_url)[datalen - (*r_fprlen + 1)] = '\0';
            }
          else
            *r_url = NULL;

          err = 0;
          goto leave;
        }
    }

 leave:
  adns_free (answer);
  adns_finish (state);
  return err;

#else /*!USE_ADNS*/

  gpg_error_t err;
  unsigned char *answer;
  int r;
  u16 count;

  *r_key = NULL;
  *r_fpr = NULL;
  *r_fprlen = 0;
  *r_url = NULL;

  /* Allocate a 64k buffer which is the limit for an DNS response.  */
  answer = xtrymalloc (65536);
  if (!answer)
    return gpg_err_make (default_errsource, gpg_err_code_from_syserror ());

  err = gpg_err_make (default_errsource, GPG_ERR_NOT_FOUND);

  r = res_query (name, C_IN, T_CERT, answer, 65536);
  /* Not too big, not too small, no errors and at least 1 answer. */
  if (r >= sizeof (HEADER) && r <= 65536
      && (((HEADER *) answer)->rcode) == NOERROR
      && (count = ntohs (((HEADER *) answer)->ancount)))
    {
      int rc;
      unsigned char *pt, *emsg;

      emsg = &answer[r];

      pt = &answer[sizeof (HEADER)];

      /* Skip over the query */

      rc = dn_skipname (pt, emsg);
      if (rc == -1)
        {
          err = gpg_err_make (default_errsource, GPG_ERR_INV_OBJ);
          goto leave;
        }
      pt += rc + QFIXEDSZ;

      /* There are several possible response types for a CERT request.
         We're interested in the PGP (a key) and IPGP (a URI) types.
         Skip all others.  TODO: A key is better than a URI since
         we've gone through all this bother to fetch it, so favor that
         if we have both PGP and IPGP? */

      while (count-- > 0 && pt < emsg)
        {
          u16 type, class, dlen, ctype;

          rc = dn_skipname (pt, emsg);  /* the name we just queried for */
          if (rc == -1)
            {
              err = gpg_err_make (default_errsource, GPG_ERR_INV_OBJ);
              goto leave;
            }

          pt += rc;

          /* Truncated message? 15 bytes takes us to the point where
             we start looking at the ctype. */
          if ((emsg - pt) < 15)
            break;

          type = *pt++ << 8;
          type |= *pt++;

          class = *pt++ << 8;
          class |= *pt++;
          /* We asked for IN and got something else !? */
          if (class != C_IN)
            break;

          /* ttl */
          pt += 4;

          /* data length */
          dlen = *pt++ << 8;
          dlen |= *pt++;

          /* We asked for CERT and got something else - might be a
             CNAME, so loop around again. */
          if (type != T_CERT)
            {
              pt += dlen;
              continue;
            }

          /* The CERT type */
          ctype = *pt++ << 8;
          ctype |= *pt++;

          /* Skip the CERT key tag and algo which we don't need. */
          pt += 3;

          dlen -= 5;

          /* 15 bytes takes us to here */

          if (ctype == CERTTYPE_PGP && dlen)
            {
              /* PGP type */
              *r_key = es_fopenmem_init (0, "rwb", pt, dlen);
              if (!*r_key)
                err = gpg_err_make (default_errsource,
                                    gpg_err_code_from_syserror ());
              else
                err = 0;
              goto leave;
            }
          else if (ctype == CERTTYPE_IPGP
                   && dlen && dlen < 1023 && dlen >= pt[0] + 1)
            {
              /* IPGP type */
              *r_fprlen = pt[0];
              if (*r_fprlen)
                {
                  *r_fpr = xtrymalloc (*r_fprlen);
                  if (!*r_fpr)
                    {
                      err = gpg_err_make (default_errsource,
                                          gpg_err_code_from_syserror ());
                      goto leave;
                    }
                  memcpy (*r_fpr, &pt[1], *r_fprlen);
                }
              else
                *r_fpr = NULL;

              if (dlen > *r_fprlen + 1)
                {
                  *r_url = xtrymalloc (dlen - (*r_fprlen + 1) + 1);
                  if (!*r_fpr)
                    {
                      err = gpg_err_make (default_errsource,
                                          gpg_err_code_from_syserror ());
                      xfree (*r_fpr);
                      *r_fpr = NULL;
                      goto leave;
                    }
                  memcpy (*r_url, &pt[*r_fprlen + 1], dlen - (*r_fprlen + 1));
                  (*r_url)[dlen - (*r_fprlen + 1)] = '\0';
                }
              else
                *r_url = NULL;

              err = 0;
              goto leave;
            }

          /* Neither type matches, so go around to the next answer. */
          pt += dlen;
        }
    }

 leave:
  xfree (answer);
  return err;

#endif /*!USE_ADNS */
#else /* !USE_DNS_CERT */
  (void)name;
  *r_key = NULL;
  *r_fpr = NULL;
  *r_fprlen = 0;
  *r_url = NULL;

  return gpg_err_make (default_errsource, GPG_ERR_NOT_SUPPORTED);
#endif
}