tls_server_start(uschar *require_ciphers, uschar *require_mac,
  uschar *require_kx, uschar *require_proto)
{
int rc;
uschar *expciphers;

/* Check for previous activation */

if (tls_active >= 0)
  {
  tls_error(US"STARTTLS received after TLS started", NULL, US"");
  smtp_printf("554 Already in TLS\r\n");
  return FAIL;
  }

/* Initialize the SSL library. If it fails, it will already have logged
the error. */

rc = tls_init(NULL, tls_dhparam, tls_certificate, tls_privatekey, NULL);
if (rc != OK) return rc;

if (!expand_check(require_ciphers, US"tls_require_ciphers", &expciphers))
  return FAIL;

/* In OpenSSL, cipher components are separated by hyphens. In GnuTLS, they
are separated by underscores. So that I can use either form in my tests, and
also for general convenience, we turn underscores into hyphens here. */

if (expciphers != NULL)
  {
  uschar *s = expciphers;
  while (*s != 0) { if (*s == '_') *s = '-'; s++; }
  DEBUG(D_tls) debug_printf("required ciphers: %s\n", expciphers);
  if (!SSL_CTX_set_cipher_list(ctx, CS expciphers))
    return tls_error(US"SSL_CTX_set_cipher_list", NULL, NULL);
  }

/* If this is a host for which certificate verification is mandatory or
optional, set up appropriately. */

tls_certificate_verified = FALSE;
verify_callback_called = FALSE;

if (verify_check_host(&tls_verify_hosts) == OK)
  {
  rc = setup_certs(tls_verify_certificates, tls_crl, NULL, FALSE);
  if (rc != OK) return rc;
  verify_optional = FALSE;
  }
else if (verify_check_host(&tls_try_verify_hosts) == OK)
  {
  rc = setup_certs(tls_verify_certificates, tls_crl, NULL, TRUE);
  if (rc != OK) return rc;
  verify_optional = TRUE;
  }

/* Prepare for new connection */

if ((ssl = SSL_new(ctx)) == NULL) return tls_error(US"SSL_new", NULL, NULL);

/* Warning: we used to SSL_clear(ssl) here, it was removed.
 *
 * With the SSL_clear(), we get strange interoperability bugs with
 * OpenSSL 1.0.1b and TLS1.1/1.2.  It looks as though this may be a bug in
 * OpenSSL itself, as a clear should not lead to inability to follow protocols.
 *
 * The SSL_clear() call is to let an existing SSL* be reused, typically after
 * session shutdown.  In this case, we have a brand new object and there's no
 * obvious reason to immediately clear it.  I'm guessing that this was
 * originally added because of incomplete initialisation which the clear fixed,
 * in some historic release.
 */

/* Set context and tell client to go ahead, except in the case of TLS startup
on connection, where outputting anything now upsets the clients and tends to
make them disconnect. We need to have an explicit fflush() here, to force out
the response. Other smtp_printf() calls do not need it, because in non-TLS
mode, the fflush() happens when smtp_getc() is called. */

SSL_set_session_id_context(ssl, sid_ctx, Ustrlen(sid_ctx));
if (!tls_on_connect)
  {
  smtp_printf("220 TLS go ahead\r\n");
  fflush(smtp_out);
  }

/* Now negotiate the TLS session. We put our own timer on it, since it seems
that the OpenSSL library doesn't. */

SSL_set_wfd(ssl, fileno(smtp_out));
SSL_set_rfd(ssl, fileno(smtp_in));
SSL_set_accept_state(ssl);

DEBUG(D_tls) debug_printf("Calling SSL_accept\n");

sigalrm_seen = FALSE;
if (smtp_receive_timeout > 0) alarm(smtp_receive_timeout);
rc = SSL_accept(ssl);
alarm(0);

if (rc <= 0)
  {
  tls_error(US"SSL_accept", NULL, sigalrm_seen ? US"timed out" : NULL);
  if (ERR_get_error() == 0)
    log_write(0, LOG_MAIN,
        "TLS client disconnected cleanly (rejected our certificate?)");
  return FAIL;
  }

DEBUG(D_tls) debug_printf("SSL_accept was successful\n");

/* TLS has been set up. Adjust the input functions to read via TLS,
and initialize things. */

construct_cipher_name(ssl);

DEBUG(D_tls)
  {
  uschar buf[2048];
  if (SSL_get_shared_ciphers(ssl, CS buf, sizeof(buf)) != NULL)
    debug_printf("Shared ciphers: %s\n", buf);
  }


ssl_xfer_buffer = store_malloc(ssl_xfer_buffer_size);
ssl_xfer_buffer_lwm = ssl_xfer_buffer_hwm = 0;
ssl_xfer_eof = ssl_xfer_error = 0;

receive_getc = tls_getc;
receive_ungetc = tls_ungetc;
receive_feof = tls_feof;
receive_ferror = tls_ferror;
receive_smtp_buffered = tls_smtp_buffered;

tls_active = fileno(smtp_out);
return OK;
}