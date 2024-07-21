fetch_next_cert_ldap (cert_fetch_context_t context,
                      unsigned char **value, size_t *valuelen)
{
  gpg_error_t err;
  unsigned char hdr[5];
  char *p, *pend;
  int n;
  int okay = 0;
  /* int is_cms = 0; */

  *value = NULL;
  *valuelen = 0;

  err = 0;
  while (!err)
    {
      err = read_buffer (context->reader, hdr, 5);
      if (err)
        break;
      n = (hdr[1] << 24)|(hdr[2]<<16)|(hdr[3]<<8)|hdr[4];
      if (*hdr == 'V' && okay)
        {
#if 0  /* That code is not yet ready.  */

          if (is_cms)
            {
              /* The certificate needs to be parsed from CMS data. */
              ksba_cms_t cms;
              ksba_stop_reason_t stopreason;
              int i;

              err = ksba_cms_new (&cms);
              if (err)
                goto leave;
              err = ksba_cms_set_reader_writer (cms, context->reader, NULL);
              if (err)
                {
                  log_error ("ksba_cms_set_reader_writer failed: %s\n",
                             gpg_strerror (err));
                  goto leave;
                }

              do
                {
                  err = ksba_cms_parse (cms, &stopreason);
                  if (err)
                    {
                      log_error ("ksba_cms_parse failed: %s\n",
                                 gpg_strerror (err));
                      goto leave;
                    }

                  if (stopreason == KSBA_SR_BEGIN_DATA)
                    log_error ("userSMIMECertificate is not "
                               "a certs-only message\n");
                }
              while (stopreason != KSBA_SR_READY);

              for (i=0; (cert=ksba_cms_get_cert (cms, i)); i++)
                {
                  check_and_store (ctrl, stats, cert, 0);
                  ksba_cert_release (cert);
                  cert = NULL;
                }
              if (!i)
                log_error ("no certificate found\n");
              else
                any = 1;
            }
          else
#endif
            {
              *value = xtrymalloc (n);
              if (!*value)
                return gpg_error_from_errno (errno);
              *valuelen = n;
              err = read_buffer (context->reader, *value, n);
              break; /* Ready or error.  */
            }
        }
      else if (!n && *hdr == 'A')
        okay = 0;
      else if (n)
        {
          if (n > context->tmpbufsize)
            {
              xfree (context->tmpbuf);
              context->tmpbufsize = 0;
              context->tmpbuf = xtrymalloc (n+1);
              if (!context->tmpbuf)
                return gpg_error_from_errno (errno);
              context->tmpbufsize = n;
            }
          err = read_buffer (context->reader, context->tmpbuf, n);
          if (err)
            break;
          if (*hdr == 'A')
            {
              p = context->tmpbuf;
              p[n] = 0; /*(we allocated one extra byte for this.)*/
              /* fixme: is_cms = 0; */
              if ( (pend = strchr (p, ';')) )
                *pend = 0; /* Strip off the extension. */
              if (!ascii_strcasecmp (p, USERCERTIFICATE))
                {
                  if (DBG_LOOKUP)
                    log_debug ("fetch_next_cert_ldap: got attribute '%s'\n",
                               USERCERTIFICATE);
                  okay = 1;
                }
              else if (!ascii_strcasecmp (p, CACERTIFICATE))
                {
                  if (DBG_LOOKUP)
                    log_debug ("fetch_next_cert_ldap: got attribute '%s'\n",
                               CACERTIFICATE);
                  okay = 1;
                }
              else if (!ascii_strcasecmp (p, X509CACERT))
                {
                  if (DBG_LOOKUP)
                    log_debug ("fetch_next_cert_ldap: got attribute '%s'\n",
                               CACERTIFICATE);
                  okay = 1;
                }
/*               else if (!ascii_strcasecmp (p, USERSMIMECERTIFICATE)) */
/*                 { */
/*                   if (DBG_LOOKUP) */
/*                     log_debug ("fetch_next_cert_ldap: got attribute '%s'\n", */
/*                                USERSMIMECERTIFICATE); */
/*                   okay = 1; */
/*                   is_cms = 1; */
/*                 } */
              else
                {
                  if (DBG_LOOKUP)
                    log_debug ("fetch_next_cert_ldap: got attribute '%s'"
                               " -  ignored\n", p);
                  okay = 0;
                }
            }
          else if (*hdr == 'E')
            {
              p = context->tmpbuf;
              p[n] = 0; /*(we allocated one extra byte for this.)*/
              if (!strcmp (p, "truncated"))
                {
                  context->truncated = 1;
                  log_info (_("ldap_search hit the size limit of"
                              " the server\n"));
                }
            }
        }
    }

  if (err)
    {
      xfree (*value);
      *value = NULL;
      *valuelen = 0;
      if (gpg_err_code (err) == GPG_ERR_EOF && context->truncated)
        {
          context->truncated = 0; /* So that the next call would return EOF. */
          err = gpg_error (GPG_ERR_TRUNCATED);
        }
    }

  return err;
}