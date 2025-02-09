wolfssl_connect_step1(struct Curl_easy *data, struct connectdata *conn,
                     int sockindex)
{
  char *ciphers;
  struct ssl_connect_data *connssl = &conn->ssl[sockindex];
  struct ssl_backend_data *backend = connssl->backend;
  SSL_METHOD* req_method = NULL;
  curl_socket_t sockfd = conn->sock[sockindex];
#ifdef HAVE_SNI
  bool sni = FALSE;
#define use_sni(x)  sni = (x)
#else
#define use_sni(x)  Curl_nop_stmt
#endif

  if(connssl->state == ssl_connection_complete)
    return CURLE_OK;

  if(SSL_CONN_CONFIG(version_max) != CURL_SSLVERSION_MAX_NONE) {
    failf(data, "wolfSSL does not support to set maximum SSL/TLS version");
    return CURLE_SSL_CONNECT_ERROR;
  }

  /* check to see if we've been told to use an explicit SSL/TLS version */
  switch(SSL_CONN_CONFIG(version)) {
  case CURL_SSLVERSION_DEFAULT:
  case CURL_SSLVERSION_TLSv1:
#if LIBWOLFSSL_VERSION_HEX >= 0x03003000 /* >= 3.3.0 */
    /* minimum protocol version is set later after the CTX object is created */
    req_method = SSLv23_client_method();
#else
    infof(data, "wolfSSL <3.3.0 cannot be configured to use TLS 1.0-1.2, "
          "TLS 1.0 is used exclusively\n");
    req_method = TLSv1_client_method();
#endif
    use_sni(TRUE);
    break;
  case CURL_SSLVERSION_TLSv1_0:
#if defined(WOLFSSL_ALLOW_TLSV10) && !defined(NO_OLD_TLS)
    req_method = TLSv1_client_method();
    use_sni(TRUE);
#else
    failf(data, "wolfSSL does not support TLS 1.0");
    return CURLE_NOT_BUILT_IN;
#endif
    break;
  case CURL_SSLVERSION_TLSv1_1:
#ifndef NO_OLD_TLS
    req_method = TLSv1_1_client_method();
    use_sni(TRUE);
#else
    failf(data, "wolfSSL does not support TLS 1.1");
    return CURLE_NOT_BUILT_IN;
#endif
    break;
  case CURL_SSLVERSION_TLSv1_2:
    req_method = TLSv1_2_client_method();
    use_sni(TRUE);
    break;
  case CURL_SSLVERSION_TLSv1_3:
#ifdef WOLFSSL_TLS13
    req_method = wolfTLSv1_3_client_method();
    use_sni(TRUE);
    break;
#else
    failf(data, "wolfSSL: TLS 1.3 is not yet supported");
    return CURLE_SSL_CONNECT_ERROR;
#endif
  case CURL_SSLVERSION_SSLv3:
#ifdef WOLFSSL_ALLOW_SSLV3
    req_method = SSLv3_client_method();
    use_sni(FALSE);
#else
    failf(data, "wolfSSL does not support SSLv3");
    return CURLE_NOT_BUILT_IN;
#endif
    break;
  case CURL_SSLVERSION_SSLv2:
    failf(data, "wolfSSL does not support SSLv2");
    return CURLE_SSL_CONNECT_ERROR;
  default:
    failf(data, "Unrecognized parameter passed via CURLOPT_SSLVERSION");
    return CURLE_SSL_CONNECT_ERROR;
  }

  if(!req_method) {
    failf(data, "SSL: couldn't create a method!");
    return CURLE_OUT_OF_MEMORY;
  }

  if(backend->ctx)
    SSL_CTX_free(backend->ctx);
  backend->ctx = SSL_CTX_new(req_method);

  if(!backend->ctx) {
    failf(data, "SSL: couldn't create a context!");
    return CURLE_OUT_OF_MEMORY;
  }

  switch(SSL_CONN_CONFIG(version)) {
  case CURL_SSLVERSION_DEFAULT:
  case CURL_SSLVERSION_TLSv1:
#if LIBWOLFSSL_VERSION_HEX > 0x03004006 /* > 3.4.6 */
    /* Versions 3.3.0 to 3.4.6 we know the minimum protocol version is
     * whatever minimum version of TLS was built in and at least TLS 1.0. For
     * later library versions that could change (eg TLS 1.0 built in but
     * defaults to TLS 1.1) so we have this short circuit evaluation to find
     * the minimum supported TLS version.
    */
    if((wolfSSL_CTX_SetMinVersion(backend->ctx, WOLFSSL_TLSV1) != 1) &&
       (wolfSSL_CTX_SetMinVersion(backend->ctx, WOLFSSL_TLSV1_1) != 1) &&
       (wolfSSL_CTX_SetMinVersion(backend->ctx, WOLFSSL_TLSV1_2) != 1)
#ifdef WOLFSSL_TLS13
       && (wolfSSL_CTX_SetMinVersion(backend->ctx, WOLFSSL_TLSV1_3) != 1)
#endif
      ) {
      failf(data, "SSL: couldn't set the minimum protocol version");
      return CURLE_SSL_CONNECT_ERROR;
    }
#endif
    break;
  }

  ciphers = SSL_CONN_CONFIG(cipher_list);
  if(ciphers) {
    if(!SSL_CTX_set_cipher_list(backend->ctx, ciphers)) {
      failf(data, "failed setting cipher list: %s", ciphers);
      return CURLE_SSL_CIPHER;
    }
    infof(data, "Cipher selection: %s\n", ciphers);
  }

#ifndef NO_FILESYSTEM
  /* load trusted cacert */
  if(SSL_CONN_CONFIG(CAfile)) {
    if(1 != SSL_CTX_load_verify_locations(backend->ctx,
                                      SSL_CONN_CONFIG(CAfile),
                                      SSL_CONN_CONFIG(CApath))) {
      if(SSL_CONN_CONFIG(verifypeer)) {
        /* Fail if we insist on successfully verifying the server. */
        failf(data, "error setting certificate verify locations:"
              " CAfile: %s CApath: %s",
              SSL_CONN_CONFIG(CAfile)?
              SSL_CONN_CONFIG(CAfile): "none",
              SSL_CONN_CONFIG(CApath)?
              SSL_CONN_CONFIG(CApath) : "none");
        return CURLE_SSL_CACERT_BADFILE;
      }
      else {
        /* Just continue with a warning if no strict certificate
           verification is required. */
        infof(data, "error setting certificate verify locations,"
              " continuing anyway:\n");
      }
    }
    else {
      /* Everything is fine. */
      infof(data, "successfully set certificate verify locations:\n");
    }
    infof(data, " CAfile: %s\n",
          SSL_CONN_CONFIG(CAfile) ? SSL_CONN_CONFIG(CAfile) : "none");
    infof(data, " CApath: %s\n",
          SSL_CONN_CONFIG(CApath) ? SSL_CONN_CONFIG(CApath) : "none");
  }

  /* Load the client certificate, and private key */
  if(SSL_SET_OPTION(primary.clientcert) && SSL_SET_OPTION(key)) {
    int file_type = do_file_type(SSL_SET_OPTION(cert_type));

    if(SSL_CTX_use_certificate_file(backend->ctx,
                                    SSL_SET_OPTION(primary.clientcert),
                                    file_type) != 1) {
      failf(data, "unable to use client certificate (no key or wrong pass"
            " phrase?)");
      return CURLE_SSL_CONNECT_ERROR;
    }

    file_type = do_file_type(SSL_SET_OPTION(key_type));
    if(SSL_CTX_use_PrivateKey_file(backend->ctx, SSL_SET_OPTION(key),
                                    file_type) != 1) {
      failf(data, "unable to set private key");
      return CURLE_SSL_CONNECT_ERROR;
    }
  }
#endif /* !NO_FILESYSTEM */

  /* SSL always tries to verify the peer, this only says whether it should
   * fail to connect if the verification fails, or if it should continue
   * anyway. In the latter case the result of the verification is checked with
   * SSL_get_verify_result() below. */
  SSL_CTX_set_verify(backend->ctx,
                     SSL_CONN_CONFIG(verifypeer)?SSL_VERIFY_PEER:
                                                 SSL_VERIFY_NONE,
                     NULL);

#ifdef HAVE_SNI
  if(sni) {
    struct in_addr addr4;
#ifdef ENABLE_IPV6
    struct in6_addr addr6;
#endif
#ifndef CURL_DISABLE_PROXY
    const char * const hostname = SSL_IS_PROXY() ? conn->http_proxy.host.name :
      conn->host.name;
#else
    const char * const hostname = conn->host.name;
#endif
    size_t hostname_len = strlen(hostname);
    if((hostname_len < USHRT_MAX) &&
       (0 == Curl_inet_pton(AF_INET, hostname, &addr4)) &&
#ifdef ENABLE_IPV6
       (0 == Curl_inet_pton(AF_INET6, hostname, &addr6)) &&
#endif
       (wolfSSL_CTX_UseSNI(backend->ctx, WOLFSSL_SNI_HOST_NAME, hostname,
                          (unsigned short)hostname_len) != 1)) {
      infof(data, "WARNING: failed to configure server name indication (SNI) "
            "TLS extension\n");
    }
  }
#endif

  /* give application a chance to interfere with SSL set up. */
  if(data->set.ssl.fsslctx) {
    CURLcode result = (*data->set.ssl.fsslctx)(data, backend->ctx,
                                               data->set.ssl.fsslctxp);
    if(result) {
      failf(data, "error signaled by ssl ctx callback");
      return result;
    }
  }
#ifdef NO_FILESYSTEM
  else if(SSL_CONN_CONFIG(verifypeer)) {
    failf(data, "SSL: Certificates can't be loaded because wolfSSL was built"
          " with \"no filesystem\". Either disable peer verification"
          " (insecure) or if you are building an application with libcurl you"
          " can load certificates via CURLOPT_SSL_CTX_FUNCTION.");
    return CURLE_SSL_CONNECT_ERROR;
  }
#endif

  /* Let's make an SSL structure */
  if(backend->handle)
    SSL_free(backend->handle);
  backend->handle = SSL_new(backend->ctx);
  if(!backend->handle) {
    failf(data, "SSL: couldn't create a context (handle)!");
    return CURLE_OUT_OF_MEMORY;
  }

#ifdef HAVE_ALPN
  if(conn->bits.tls_enable_alpn) {
    char protocols[128];
    *protocols = '\0';

    /* wolfSSL's ALPN protocol name list format is a comma separated string of
       protocols in descending order of preference, eg: "h2,http/1.1" */

#ifdef USE_NGHTTP2
    if(data->state.httpversion >= CURL_HTTP_VERSION_2) {
      strcpy(protocols + strlen(protocols), NGHTTP2_PROTO_VERSION_ID ",");
      infof(data, "ALPN, offering %s\n", NGHTTP2_PROTO_VERSION_ID);
    }
#endif

    strcpy(protocols + strlen(protocols), ALPN_HTTP_1_1);
    infof(data, "ALPN, offering %s\n", ALPN_HTTP_1_1);

    if(wolfSSL_UseALPN(backend->handle, protocols,
                       (unsigned)strlen(protocols),
                       WOLFSSL_ALPN_CONTINUE_ON_MISMATCH) != SSL_SUCCESS) {
      failf(data, "SSL: failed setting ALPN protocols");
      return CURLE_SSL_CONNECT_ERROR;
    }
  }
#endif /* HAVE_ALPN */

#ifdef OPENSSL_EXTRA
  if(Curl_tls_keylog_enabled()) {
    /* Ensure the Client Random is preserved. */
    wolfSSL_KeepArrays(backend->handle);
#if defined(HAVE_SECRET_CALLBACK) && defined(WOLFSSL_TLS13)
    wolfSSL_set_tls13_secret_cb(backend->handle,
                                wolfssl_tls13_secret_callback, NULL);
#endif
  }
#endif /* OPENSSL_EXTRA */

#ifdef HAVE_SECURE_RENEGOTIATION
  if(wolfSSL_UseSecureRenegotiation(backend->handle) != SSL_SUCCESS) {
    failf(data, "SSL: failed setting secure renegotiation");
    return CURLE_SSL_CONNECT_ERROR;
  }
#endif /* HAVE_SECURE_RENEGOTIATION */

  /* Check if there's a cached ID we can/should use here! */
  if(SSL_SET_OPTION(primary.sessionid)) {
    void *ssl_sessionid = NULL;

    Curl_ssl_sessionid_lock(data);
    if(!Curl_ssl_getsessionid(data, conn, &ssl_sessionid, NULL, sockindex)) {
      /* we got a session id, use it! */
      if(!SSL_set_session(backend->handle, ssl_sessionid)) {
        char error_buffer[WOLFSSL_MAX_ERROR_SZ];
        Curl_ssl_sessionid_unlock(data);
        failf(data, "SSL: SSL_set_session failed: %s",
              ERR_error_string(SSL_get_error(backend->handle, 0),
                               error_buffer));
        return CURLE_SSL_CONNECT_ERROR;
      }
      /* Informational message */
      infof(data, "SSL re-using session ID\n");
    }
    Curl_ssl_sessionid_unlock(data);
  }

  /* pass the raw socket into the SSL layer */
  if(!SSL_set_fd(backend->handle, (int)sockfd)) {
    failf(data, "SSL: SSL_set_fd failed");
    return CURLE_SSL_CONNECT_ERROR;
  }

  connssl->connecting_state = ssl_connect_2;
  return CURLE_OK;
}