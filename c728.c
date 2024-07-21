tor_tls_context_new(crypto_pk_env_t *identity, unsigned int key_lifetime)
{
  crypto_pk_env_t *rsa = NULL;
  EVP_PKEY *pkey = NULL;
  tor_tls_context_t *result = NULL;
  X509 *cert = NULL, *idcert = NULL;
  char *nickname = NULL, *nn2 = NULL;

  tor_tls_init();
  nickname = crypto_random_hostname(8, 20, "www.", ".net");
  nn2 = crypto_random_hostname(8, 20, "www.", ".net");

  /* Generate short-term RSA key. */
  if (!(rsa = crypto_new_pk_env()))
    goto error;
  if (crypto_pk_generate_key(rsa)<0)
    goto error;
  /* Create certificate signed by identity key. */
  cert = tor_tls_create_certificate(rsa, identity, nickname, nn2,
                                    key_lifetime);
  /* Create self-signed certificate for identity key. */
  idcert = tor_tls_create_certificate(identity, identity, nn2, nn2,
                                      IDENTITY_CERT_LIFETIME);
  if (!cert || !idcert) {
    log(LOG_WARN, LD_CRYPTO, "Error creating certificate");
    goto error;
  }

  result = tor_malloc_zero(sizeof(tor_tls_context_t));
  result->refcnt = 1;
  result->my_cert = X509_dup(cert);
  result->my_id_cert = X509_dup(idcert);
  result->key = crypto_pk_dup_key(rsa);

#ifdef EVERYONE_HAS_AES
  /* Tell OpenSSL to only use TLS1 */
  if (!(result->ctx = SSL_CTX_new(TLSv1_method())))
    goto error;
#else
  /* Tell OpenSSL to use SSL3 or TLS1 but not SSL2. */
  if (!(result->ctx = SSL_CTX_new(SSLv23_method())))
    goto error;
  SSL_CTX_set_options(result->ctx, SSL_OP_NO_SSLv2);
#endif
  SSL_CTX_set_options(result->ctx, SSL_OP_SINGLE_DH_USE);

#ifdef SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION
  SSL_CTX_set_options(result->ctx,
                      SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION);
#endif
  /* Yes, we know what we are doing here.  No, we do not treat a renegotiation
   * as authenticating any earlier-received data.
   */
  if (use_unsafe_renegotiation_op) {
    SSL_CTX_set_options(result->ctx,
                        SSL_OP_ALLOW_UNSAFE_LEGACY_RENEGOTIATION);
  }
  /* Don't actually allow compression; it uses ram and time, but the data
   * we transmit is all encrypted anyway. */
  if (result->ctx->comp_methods)
    result->ctx->comp_methods = NULL;
#ifdef SSL_MODE_RELEASE_BUFFERS
  SSL_CTX_set_mode(result->ctx, SSL_MODE_RELEASE_BUFFERS);
#endif
  if (cert && !SSL_CTX_use_certificate(result->ctx,cert))
    goto error;
  X509_free(cert); /* We just added a reference to cert. */
  cert=NULL;
  if (idcert) {
    X509_STORE *s = SSL_CTX_get_cert_store(result->ctx);
    tor_assert(s);
    X509_STORE_add_cert(s, idcert);
    X509_free(idcert); /* The context now owns the reference to idcert */
    idcert = NULL;
  }
  SSL_CTX_set_session_cache_mode(result->ctx, SSL_SESS_CACHE_OFF);
  tor_assert(rsa);
  if (!(pkey = _crypto_pk_env_get_evp_pkey(rsa,1)))
    goto error;
  if (!SSL_CTX_use_PrivateKey(result->ctx, pkey))
    goto error;
  EVP_PKEY_free(pkey);
  pkey = NULL;
  if (!SSL_CTX_check_private_key(result->ctx))
    goto error;
  {
    crypto_dh_env_t *dh = crypto_dh_new(DH_TYPE_TLS);
    SSL_CTX_set_tmp_dh(result->ctx, _crypto_dh_env_get_dh(dh));
    crypto_dh_free(dh);
  }
  SSL_CTX_set_verify(result->ctx, SSL_VERIFY_PEER,
                     always_accept_verify_cb);
  /* let us realloc bufs that we're writing from */
  SSL_CTX_set_mode(result->ctx, SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);

  if (rsa)
    crypto_free_pk_env(rsa);
  tor_free(nickname);
  tor_free(nn2);
  return result;

 error:
  tls_log_errors(NULL, LOG_WARN, "creating TLS context");
  tor_free(nickname);
  tor_free(nn2);
  if (pkey)
    EVP_PKEY_free(pkey);
  if (rsa)
    crypto_free_pk_env(rsa);
  if (result)
    tor_tls_context_decref(result);
  if (cert)
    X509_free(cert);
  if (idcert)
    X509_free(idcert);
  return NULL;
}