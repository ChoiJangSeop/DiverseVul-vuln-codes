tls_init(host_item *host, uschar *dhparam, uschar *certificate,
  uschar *privatekey, address_item *addr)
{
long init_options;
BOOL okay;

SSL_load_error_strings();          /* basic set up */
OpenSSL_add_ssl_algorithms();

#if (OPENSSL_VERSION_NUMBER >= 0x0090800fL) && !defined(OPENSSL_NO_SHA256)
/* SHA256 is becoming ever more popular. This makes sure it gets added to the
list of available digests. */
EVP_add_digest(EVP_sha256());
#endif

/* Create a context */

ctx = SSL_CTX_new((host == NULL)?
  SSLv23_server_method() : SSLv23_client_method());

if (ctx == NULL) return tls_error(US"SSL_CTX_new", host, NULL);

/* It turns out that we need to seed the random number generator this early in
order to get the full complement of ciphers to work. It took me roughly a day
of work to discover this by experiment.

On systems that have /dev/urandom, SSL may automatically seed itself from
there. Otherwise, we have to make something up as best we can. Double check
afterwards. */

if (!RAND_status())
  {
  randstuff r;
  gettimeofday(&r.tv, NULL);
  r.p = getpid();

  RAND_seed((uschar *)(&r), sizeof(r));
  RAND_seed((uschar *)big_buffer, big_buffer_size);
  if (addr != NULL) RAND_seed((uschar *)addr, sizeof(addr));

  if (!RAND_status())
    return tls_error(US"RAND_status", host,
      US"unable to seed random number generator");
  }

/* Set up the information callback, which outputs if debugging is at a suitable
level. */

SSL_CTX_set_info_callback(ctx, (void (*)())info_callback);

/* Automatically re-try reads/writes after renegotiation. */
(void) SSL_CTX_set_mode(ctx, SSL_MODE_AUTO_RETRY);

/* Apply administrator-supplied work-arounds.
Historically we applied just one requested option,
SSL_OP_DONT_INSERT_EMPTY_FRAGMENTS, but when bug 994 requested a second, we
moved to an administrator-controlled list of options to specify and
grandfathered in the first one as the default value for "openssl_options".

No OpenSSL version number checks: the options we accept depend upon the
availability of the option value macros from OpenSSL.  */

okay = tls_openssl_options_parse(openssl_options, &init_options);
if (!okay)
  return tls_error(US"openssl_options parsing failed", host, NULL);

if (init_options)
  {
  DEBUG(D_tls) debug_printf("setting SSL CTX options: %#lx\n", init_options);
  if (!(SSL_CTX_set_options(ctx, init_options)))
    return tls_error(string_sprintf(
          "SSL_CTX_set_option(%#lx)", init_options), host, NULL);
  }
else
  DEBUG(D_tls) debug_printf("no SSL CTX options to set\n");

/* Initialize with DH parameters if supplied */

if (!init_dh(dhparam, host)) return DEFER;

/* Set up certificate and key */

if (certificate != NULL)
  {
  uschar *expanded;
  if (!expand_check(certificate, US"tls_certificate", &expanded))
    return DEFER;

  if (expanded != NULL)
    {
    DEBUG(D_tls) debug_printf("tls_certificate file %s\n", expanded);
    if (!SSL_CTX_use_certificate_chain_file(ctx, CS expanded))
      return tls_error(string_sprintf(
        "SSL_CTX_use_certificate_chain_file file=%s", expanded), host, NULL);
    }

  if (privatekey != NULL &&
      !expand_check(privatekey, US"tls_privatekey", &expanded))
    return DEFER;

  /* If expansion was forced to fail, key_expanded will be NULL. If the result
  of the expansion is an empty string, ignore it also, and assume the private
  key is in the same file as the certificate. */

  if (expanded != NULL && *expanded != 0)
    {
    DEBUG(D_tls) debug_printf("tls_privatekey file %s\n", expanded);
    if (!SSL_CTX_use_PrivateKey_file(ctx, CS expanded, SSL_FILETYPE_PEM))
      return tls_error(string_sprintf(
        "SSL_CTX_use_PrivateKey_file file=%s", expanded), host, NULL);
    }
  }

/* Set up the RSA callback */

SSL_CTX_set_tmp_rsa_callback(ctx, rsa_callback);

/* Finally, set the timeout, and we are done */

SSL_CTX_set_timeout(ctx, ssl_session_timeout);
DEBUG(D_tls) debug_printf("Initialized TLS\n");
return OK;
}