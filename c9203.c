int mutt_ssl_starttls (CONNECTION* conn)
{
  sslsockdata* ssldata;
  int maxbits;
  long ssl_options = 0;

  if (ssl_init())
    goto bail;

  ssldata = (sslsockdata*) safe_calloc (1, sizeof (sslsockdata));
  /* the ssl_use_xxx protocol options don't apply. We must use TLS in TLS.
   *
   * However, we need to be able to negotiate amongst various TLS versions,
   * which at present can only be done with the SSLv23_client_method;
   * TLSv1_client_method gives us explicitly TLSv1.0, not 1.1 or 1.2 (True as
   * of OpenSSL 1.0.1c)
   */
  if (! (ssldata->ctx = SSL_CTX_new (SSLv23_client_method())))
  {
    dprint (1, (debugfile, "mutt_ssl_starttls: Error allocating SSL_CTX\n"));
    goto bail_ssldata;
  }
#ifdef SSL_OP_NO_TLSv1_3
  if (!option(OPTTLSV1_3))
    ssl_options |= SSL_OP_NO_TLSv1_3;
#endif
#ifdef SSL_OP_NO_TLSv1_2
  if (!option(OPTTLSV1_2))
    ssl_options |= SSL_OP_NO_TLSv1_2;
#endif
#ifdef SSL_OP_NO_TLSv1_1
  if (!option(OPTTLSV1_1))
    ssl_options |= SSL_OP_NO_TLSv1_1;
#endif
#ifdef SSL_OP_NO_TLSv1
  if (!option(OPTTLSV1))
    ssl_options |= SSL_OP_NO_TLSv1;
#endif
  /* these are always set */
#ifdef SSL_OP_NO_SSLv3
  ssl_options |= SSL_OP_NO_SSLv3;
#endif
#ifdef SSL_OP_NO_SSLv2
  ssl_options |= SSL_OP_NO_SSLv2;
#endif
  if (! SSL_CTX_set_options(ssldata->ctx, ssl_options))
  {
    dprint(1, (debugfile, "mutt_ssl_starttls: Error setting options to %ld\n", ssl_options));
    goto bail_ctx;
  }

  if (option (OPTSSLSYSTEMCERTS))
  {
    if (! SSL_CTX_set_default_verify_paths (ssldata->ctx))
    {
      dprint (1, (debugfile, "mutt_ssl_starttls: Error setting default verify paths\n"));
      goto bail_ctx;
    }
  }

  if (SslCertFile && !ssl_load_certificates (ssldata->ctx))
    dprint (1, (debugfile, "mutt_ssl_starttls: Error loading trusted certificates\n"));

  ssl_get_client_cert(ssldata, conn);

  if (SslCiphers)
  {
    if (!SSL_CTX_set_cipher_list (ssldata->ctx, SslCiphers))
    {
      dprint (1, (debugfile, "mutt_ssl_starttls: Could not select preferred ciphers\n"));
      goto bail_ctx;
    }
  }

  if (ssl_set_verify_partial (ssldata->ctx))
  {
    mutt_error (_("Warning: error enabling ssl_verify_partial_chains"));
    mutt_sleep (2);
  }

  if (! (ssldata->ssl = SSL_new (ssldata->ctx)))
  {
    dprint (1, (debugfile, "mutt_ssl_starttls: Error allocating SSL\n"));
    goto bail_ctx;
  }

  if (SSL_set_fd (ssldata->ssl, conn->fd) != 1)
  {
    dprint (1, (debugfile, "mutt_ssl_starttls: Error setting fd\n"));
    goto bail_ssl;
  }

  if (ssl_negotiate (conn, ssldata))
    goto bail_ssl;

  ssldata->isopen = 1;

  /* hmm. watch out if we're starting TLS over any method other than raw. */
  conn->sockdata = ssldata;
  conn->conn_read = ssl_socket_read;
  conn->conn_write = ssl_socket_write;
  conn->conn_close = tls_close;
  conn->conn_poll = ssl_socket_poll;

  conn->ssf = SSL_CIPHER_get_bits (SSL_get_current_cipher (ssldata->ssl),
                                   &maxbits);

  return 0;

bail_ssl:
  SSL_free (ssldata->ssl);
  ssldata->ssl = 0;
bail_ctx:
  SSL_CTX_free (ssldata->ctx);
  ssldata->ctx = 0;
bail_ssldata:
  FREE (&ssldata);
bail:
  return -1;
}