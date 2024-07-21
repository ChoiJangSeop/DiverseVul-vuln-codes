cmd_readkey (assuan_context_t ctx, char *line)
{
  ctrl_t ctrl = assuan_get_pointer (ctx);
  int rc;
  unsigned char *cert = NULL;
  size_t ncert, n;
  ksba_cert_t kc = NULL;
  ksba_sexp_t p;
  unsigned char *pk;
  size_t pklen;

  if ((rc = open_card (ctrl, NULL)))
    return rc;

  line = xstrdup (line); /* Need a copy of the line. */
  /* If the application supports the READKEY function we use that.
     Otherwise we use the old way by extracting it from the
     certificate.  */
  rc = app_readkey (ctrl->app_ctx, line, &pk, &pklen);
  if (!rc)
    { /* Yeah, got that key - send it back.  */
      rc = assuan_send_data (ctx, pk, pklen);
      xfree (pk);
      xfree (line);
      line = NULL;
      goto leave;
    }

  if (gpg_err_code (rc) != GPG_ERR_UNSUPPORTED_OPERATION)
    log_error ("app_readkey failed: %s\n", gpg_strerror (rc));
  else
    {
      rc = app_readcert (ctrl->app_ctx, line, &cert, &ncert);
      if (rc)
        log_error ("app_readcert failed: %s\n", gpg_strerror (rc));
    }
  xfree (line);
  line = NULL;
  if (rc)
    goto leave;

  rc = ksba_cert_new (&kc);
  if (rc)
    {
      xfree (cert);
      goto leave;
    }
  rc = ksba_cert_init_from_mem (kc, cert, ncert);
  if (rc)
    {
      log_error ("failed to parse the certificate: %s\n", gpg_strerror (rc));
      goto leave;
    }

  p = ksba_cert_get_public_key (kc);
  if (!p)
    {
      rc = gpg_error (GPG_ERR_NO_PUBKEY);
      goto leave;
    }

  n = gcry_sexp_canon_len (p, 0, NULL, NULL);
  rc = assuan_send_data (ctx, p, n);
  xfree (p);


 leave:
  ksba_cert_release (kc);
  xfree (cert);
  TEST_CARD_REMOVAL (ctrl, rc);
  return rc;
}