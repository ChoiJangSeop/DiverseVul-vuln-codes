setup_certs(uschar *certs, uschar *crl, host_item *host, BOOL optional)
{
uschar *expcerts, *expcrl;

if (!expand_check(certs, US"tls_verify_certificates", &expcerts))
  return DEFER;

if (expcerts != NULL)
  {
  struct stat statbuf;
  if (!SSL_CTX_set_default_verify_paths(ctx))
    return tls_error(US"SSL_CTX_set_default_verify_paths", host, NULL);

  if (Ustat(expcerts, &statbuf) < 0)
    {
    log_write(0, LOG_MAIN|LOG_PANIC,
      "failed to stat %s for certificates", expcerts);
    return DEFER;
    }
  else
    {
    uschar *file, *dir;
    if ((statbuf.st_mode & S_IFMT) == S_IFDIR)
      { file = NULL; dir = expcerts; }
    else
      { file = expcerts; dir = NULL; }

    /* If a certificate file is empty, the next function fails with an
    unhelpful error message. If we skip it, we get the correct behaviour (no
    certificates are recognized, but the error message is still misleading (it
    says no certificate was supplied.) But this is better. */

    if ((file == NULL || statbuf.st_size > 0) &&
          !SSL_CTX_load_verify_locations(ctx, CS file, CS dir))
      return tls_error(US"SSL_CTX_load_verify_locations", host, NULL);

    if (file != NULL)
      {
      SSL_CTX_set_client_CA_list(ctx, SSL_load_client_CA_file(CS file));
      }
    }

  /* Handle a certificate revocation list. */

  #if OPENSSL_VERSION_NUMBER > 0x00907000L

  /* This bit of code is now the version supplied by Lars Mainka. (I have
   * merely reformatted it into the Exim code style.)

   * "From here I changed the code to add support for multiple crl's
   * in pem format in one file or to support hashed directory entries in
   * pem format instead of a file. This method now uses the library function
   * X509_STORE_load_locations to add the CRL location to the SSL context.
   * OpenSSL will then handle the verify against CA certs and CRLs by
   * itself in the verify callback." */

  if (!expand_check(crl, US"tls_crl", &expcrl)) return DEFER;
  if (expcrl != NULL && *expcrl != 0)
    {
    struct stat statbufcrl;
    if (Ustat(expcrl, &statbufcrl) < 0)
      {
      log_write(0, LOG_MAIN|LOG_PANIC,
        "failed to stat %s for certificates revocation lists", expcrl);
      return DEFER;
      }
    else
      {
      /* is it a file or directory? */
      uschar *file, *dir;
      X509_STORE *cvstore = SSL_CTX_get_cert_store(ctx);
      if ((statbufcrl.st_mode & S_IFMT) == S_IFDIR)
        {
        file = NULL;
        dir = expcrl;
        DEBUG(D_tls) debug_printf("SSL CRL value is a directory %s\n", dir);
        }
      else
        {
        file = expcrl;
        dir = NULL;
        DEBUG(D_tls) debug_printf("SSL CRL value is a file %s\n", file);
        }
      if (X509_STORE_load_locations(cvstore, CS file, CS dir) == 0)
        return tls_error(US"X509_STORE_load_locations", host, NULL);

      /* setting the flags to check against the complete crl chain */

      X509_STORE_set_flags(cvstore,
        X509_V_FLAG_CRL_CHECK|X509_V_FLAG_CRL_CHECK_ALL);
      }
    }

  #endif  /* OPENSSL_VERSION_NUMBER > 0x00907000L */

  /* If verification is optional, don't fail if no certificate */

  SSL_CTX_set_verify(ctx,
    SSL_VERIFY_PEER | (optional? 0 : SSL_VERIFY_FAIL_IF_NO_PEER_CERT),
    verify_callback);
  }

return OK;
}