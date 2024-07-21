gtls_connect_step1(struct Curl_easy *data,
                   struct connectdata *conn,
                   int sockindex)
{
  struct ssl_connect_data *connssl = &conn->ssl[sockindex];
  struct ssl_backend_data *backend = connssl->backend;
  unsigned int init_flags;
  gnutls_session_t session;
  int rc;
  bool sni = TRUE; /* default is SNI enabled */
  void *transport_ptr = NULL;
  gnutls_push_func gnutls_transport_push = NULL;
  gnutls_pull_func gnutls_transport_pull = NULL;
#ifdef ENABLE_IPV6
  struct in6_addr addr;
#else
  struct in_addr addr;
#endif
  const char *prioritylist;
  const char *err = NULL;
  const char * const hostname = SSL_HOST_NAME();
  long * const certverifyresult = &SSL_SET_OPTION_LVALUE(certverifyresult);

  if(connssl->state == ssl_connection_complete)
    /* to make us tolerant against being called more than once for the
       same connection */
    return CURLE_OK;

  if(!gtls_inited)
    gtls_init();

  /* Initialize certverifyresult to OK */
  *certverifyresult = 0;

  if(SSL_CONN_CONFIG(version) == CURL_SSLVERSION_SSLv2) {
    failf(data, "GnuTLS does not support SSLv2");
    return CURLE_SSL_CONNECT_ERROR;
  }
  else if(SSL_CONN_CONFIG(version) == CURL_SSLVERSION_SSLv3)
    sni = FALSE; /* SSLv3 has no SNI */

  /* allocate a cred struct */
  rc = gnutls_certificate_allocate_credentials(&backend->cred);
  if(rc != GNUTLS_E_SUCCESS) {
    failf(data, "gnutls_cert_all_cred() failed: %s", gnutls_strerror(rc));
    return CURLE_SSL_CONNECT_ERROR;
  }

#ifdef HAVE_GNUTLS_SRP
  if(SSL_SET_OPTION(authtype) == CURL_TLSAUTH_SRP) {
    infof(data, "Using TLS-SRP username: %s\n", SSL_SET_OPTION(username));

    rc = gnutls_srp_allocate_client_credentials(
           &backend->srp_client_cred);
    if(rc != GNUTLS_E_SUCCESS) {
      failf(data, "gnutls_srp_allocate_client_cred() failed: %s",
            gnutls_strerror(rc));
      return CURLE_OUT_OF_MEMORY;
    }

    rc = gnutls_srp_set_client_credentials(backend->srp_client_cred,
                                           SSL_SET_OPTION(username),
                                           SSL_SET_OPTION(password));
    if(rc != GNUTLS_E_SUCCESS) {
      failf(data, "gnutls_srp_set_client_cred() failed: %s",
            gnutls_strerror(rc));
      return CURLE_BAD_FUNCTION_ARGUMENT;
    }
  }
#endif

  if(SSL_CONN_CONFIG(CAfile)) {
    /* set the trusted CA cert bundle file */
    gnutls_certificate_set_verify_flags(backend->cred,
                                        GNUTLS_VERIFY_ALLOW_X509_V1_CA_CRT);

    rc = gnutls_certificate_set_x509_trust_file(backend->cred,
                                                SSL_CONN_CONFIG(CAfile),
                                                GNUTLS_X509_FMT_PEM);
    if(rc < 0) {
      infof(data, "error reading ca cert file %s (%s)\n",
            SSL_CONN_CONFIG(CAfile), gnutls_strerror(rc));
      if(SSL_CONN_CONFIG(verifypeer)) {
        *certverifyresult = rc;
        return CURLE_SSL_CACERT_BADFILE;
      }
    }
    else
      infof(data, "found %d certificates in %s\n", rc,
            SSL_CONN_CONFIG(CAfile));
  }

  if(SSL_CONN_CONFIG(CApath)) {
    /* set the trusted CA cert directory */
    rc = gnutls_certificate_set_x509_trust_dir(backend->cred,
                                               SSL_CONN_CONFIG(CApath),
                                               GNUTLS_X509_FMT_PEM);
    if(rc < 0) {
      infof(data, "error reading ca cert file %s (%s)\n",
            SSL_CONN_CONFIG(CApath), gnutls_strerror(rc));
      if(SSL_CONN_CONFIG(verifypeer)) {
        *certverifyresult = rc;
        return CURLE_SSL_CACERT_BADFILE;
      }
    }
    else
      infof(data, "found %d certificates in %s\n",
            rc, SSL_CONN_CONFIG(CApath));
  }

#ifdef CURL_CA_FALLBACK
  /* use system ca certificate store as fallback */
  if(SSL_CONN_CONFIG(verifypeer) &&
     !(SSL_CONN_CONFIG(CAfile) || SSL_CONN_CONFIG(CApath))) {
    gnutls_certificate_set_x509_system_trust(backend->cred);
  }
#endif

  if(SSL_SET_OPTION(CRLfile)) {
    /* set the CRL list file */
    rc = gnutls_certificate_set_x509_crl_file(backend->cred,
                                              SSL_SET_OPTION(CRLfile),
                                              GNUTLS_X509_FMT_PEM);
    if(rc < 0) {
      failf(data, "error reading crl file %s (%s)",
            SSL_SET_OPTION(CRLfile), gnutls_strerror(rc));
      return CURLE_SSL_CRL_BADFILE;
    }
    else
      infof(data, "found %d CRL in %s\n",
            rc, SSL_SET_OPTION(CRLfile));
  }

  /* Initialize TLS session as a client */
  init_flags = GNUTLS_CLIENT;

#if defined(GNUTLS_FORCE_CLIENT_CERT)
  init_flags |= GNUTLS_FORCE_CLIENT_CERT;
#endif

#if defined(GNUTLS_NO_TICKETS)
  /* Disable TLS session tickets */
  init_flags |= GNUTLS_NO_TICKETS;
#endif

  rc = gnutls_init(&backend->session, init_flags);
  if(rc != GNUTLS_E_SUCCESS) {
    failf(data, "gnutls_init() failed: %d", rc);
    return CURLE_SSL_CONNECT_ERROR;
  }

  /* convenient assign */
  session = backend->session;

  if((0 == Curl_inet_pton(AF_INET, hostname, &addr)) &&
#ifdef ENABLE_IPV6
     (0 == Curl_inet_pton(AF_INET6, hostname, &addr)) &&
#endif
     sni &&
     (gnutls_server_name_set(session, GNUTLS_NAME_DNS, hostname,
                             strlen(hostname)) < 0))
    infof(data, "WARNING: failed to configure server name indication (SNI) "
          "TLS extension\n");

  /* Use default priorities */
  rc = gnutls_set_default_priority(session);
  if(rc != GNUTLS_E_SUCCESS)
    return CURLE_SSL_CONNECT_ERROR;

  /* Ensure +SRP comes at the *end* of all relevant strings so that it can be
   * removed if a run-time error indicates that SRP is not supported by this
   * GnuTLS version */
  switch(SSL_CONN_CONFIG(version)) {
    case CURL_SSLVERSION_SSLv3:
      prioritylist = GNUTLS_CIPHERS ":-VERS-TLS-ALL:+VERS-SSL3.0";
      break;
    case CURL_SSLVERSION_DEFAULT:
    case CURL_SSLVERSION_TLSv1:
      prioritylist = GNUTLS_CIPHERS ":-VERS-SSL3.0"
#ifdef HAS_TLS13
                     ":+VERS-TLS1.3"
#endif
                     ;
      break;
    case CURL_SSLVERSION_TLSv1_0:
    case CURL_SSLVERSION_TLSv1_1:
    case CURL_SSLVERSION_TLSv1_2:
    case CURL_SSLVERSION_TLSv1_3:
      {
        CURLcode result = set_ssl_version_min_max(&prioritylist, data);
        if(result != CURLE_OK)
          return result;
        break;
      }
    case CURL_SSLVERSION_SSLv2:
      failf(data, "GnuTLS does not support SSLv2");
      return CURLE_SSL_CONNECT_ERROR;
    default:
      failf(data, "Unrecognized parameter passed via CURLOPT_SSLVERSION");
      return CURLE_SSL_CONNECT_ERROR;
  }

#ifdef HAVE_GNUTLS_SRP
  /* Only add SRP to the cipher list if SRP is requested. Otherwise
   * GnuTLS will disable TLS 1.3 support. */
  if(SSL_SET_OPTION(authtype) == CURL_TLSAUTH_SRP) {
    size_t len = strlen(prioritylist);

    char *prioritysrp = malloc(len + sizeof(GNUTLS_SRP) + 1);
    if(!prioritysrp)
      return CURLE_OUT_OF_MEMORY;
    strcpy(prioritysrp, prioritylist);
    strcpy(prioritysrp + len, ":" GNUTLS_SRP);

    rc = gnutls_priority_set_direct(session, prioritysrp, &err);
    free(prioritysrp);

    if((rc == GNUTLS_E_INVALID_REQUEST) && err) {
      infof(data, "This GnuTLS does not support SRP\n");
    }
  }
  else {
#endif
    rc = gnutls_priority_set_direct(session, prioritylist, &err);
#ifdef HAVE_GNUTLS_SRP
  }
#endif

  if(rc != GNUTLS_E_SUCCESS) {
    failf(data, "Error %d setting GnuTLS cipher list starting with %s",
          rc, err);
    return CURLE_SSL_CONNECT_ERROR;
  }

  if(conn->bits.tls_enable_alpn) {
    int cur = 0;
    gnutls_datum_t protocols[2];

#ifdef USE_NGHTTP2
    if(data->state.httpversion >= CURL_HTTP_VERSION_2
#ifndef CURL_DISABLE_PROXY
       && (!SSL_IS_PROXY() || !conn->bits.tunnel_proxy)
#endif
       ) {
      protocols[cur].data = (unsigned char *)NGHTTP2_PROTO_VERSION_ID;
      protocols[cur].size = NGHTTP2_PROTO_VERSION_ID_LEN;
      cur++;
      infof(data, "ALPN, offering %s\n", NGHTTP2_PROTO_VERSION_ID);
    }
#endif

    protocols[cur].data = (unsigned char *)ALPN_HTTP_1_1;
    protocols[cur].size = ALPN_HTTP_1_1_LENGTH;
    cur++;
    infof(data, "ALPN, offering %s\n", ALPN_HTTP_1_1);

    gnutls_alpn_set_protocols(session, protocols, cur, 0);
  }

  if(SSL_SET_OPTION(primary.clientcert)) {
    if(SSL_SET_OPTION(key_passwd)) {
      const unsigned int supported_key_encryption_algorithms =
        GNUTLS_PKCS_USE_PKCS12_3DES | GNUTLS_PKCS_USE_PKCS12_ARCFOUR |
        GNUTLS_PKCS_USE_PKCS12_RC2_40 | GNUTLS_PKCS_USE_PBES2_3DES |
        GNUTLS_PKCS_USE_PBES2_AES_128 | GNUTLS_PKCS_USE_PBES2_AES_192 |
        GNUTLS_PKCS_USE_PBES2_AES_256;
      rc = gnutls_certificate_set_x509_key_file2(
           backend->cred,
           SSL_SET_OPTION(primary.clientcert),
           SSL_SET_OPTION(key) ?
           SSL_SET_OPTION(key) : SSL_SET_OPTION(primary.clientcert),
           do_file_type(SSL_SET_OPTION(cert_type)),
           SSL_SET_OPTION(key_passwd),
           supported_key_encryption_algorithms);
      if(rc != GNUTLS_E_SUCCESS) {
        failf(data,
              "error reading X.509 potentially-encrypted key file: %s",
              gnutls_strerror(rc));
        return CURLE_SSL_CONNECT_ERROR;
      }
    }
    else {
      if(gnutls_certificate_set_x509_key_file(
           backend->cred,
           SSL_SET_OPTION(primary.clientcert),
           SSL_SET_OPTION(key) ?
           SSL_SET_OPTION(key) : SSL_SET_OPTION(primary.clientcert),
           do_file_type(SSL_SET_OPTION(cert_type)) ) !=
         GNUTLS_E_SUCCESS) {
        failf(data, "error reading X.509 key or certificate file");
        return CURLE_SSL_CONNECT_ERROR;
      }
    }
  }

#ifdef HAVE_GNUTLS_SRP
  /* put the credentials to the current session */
  if(SSL_SET_OPTION(authtype) == CURL_TLSAUTH_SRP) {
    rc = gnutls_credentials_set(session, GNUTLS_CRD_SRP,
                                backend->srp_client_cred);
    if(rc != GNUTLS_E_SUCCESS) {
      failf(data, "gnutls_credentials_set() failed: %s", gnutls_strerror(rc));
      return CURLE_SSL_CONNECT_ERROR;
    }
  }
  else
#endif
  {
    rc = gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE,
                                backend->cred);
    if(rc != GNUTLS_E_SUCCESS) {
      failf(data, "gnutls_credentials_set() failed: %s", gnutls_strerror(rc));
      return CURLE_SSL_CONNECT_ERROR;
    }
  }

#ifndef CURL_DISABLE_PROXY
  if(conn->proxy_ssl[sockindex].use) {
    transport_ptr = conn->proxy_ssl[sockindex].backend->session;
    gnutls_transport_push = gtls_push_ssl;
    gnutls_transport_pull = gtls_pull_ssl;
  }
  else
#endif
  {
    /* file descriptor for the socket */
    transport_ptr = &conn->sock[sockindex];
    gnutls_transport_push = gtls_push;
    gnutls_transport_pull = gtls_pull;
  }

  /* set the connection handle */
  gnutls_transport_set_ptr(session, transport_ptr);

  /* register callback functions to send and receive data. */
  gnutls_transport_set_push_function(session, gnutls_transport_push);
  gnutls_transport_set_pull_function(session, gnutls_transport_pull);

  if(SSL_CONN_CONFIG(verifystatus)) {
    rc = gnutls_ocsp_status_request_enable_client(session, NULL, 0, NULL);
    if(rc != GNUTLS_E_SUCCESS) {
      failf(data, "gnutls_ocsp_status_request_enable_client() failed: %d", rc);
      return CURLE_SSL_CONNECT_ERROR;
    }
  }

  /* This might be a reconnect, so we check for a session ID in the cache
     to speed up things */
  if(SSL_SET_OPTION(primary.sessionid)) {
    void *ssl_sessionid;
    size_t ssl_idsize;

    Curl_ssl_sessionid_lock(data);
    if(!Curl_ssl_getsessionid(data, conn,
                              &ssl_sessionid, &ssl_idsize, sockindex)) {
      /* we got a session id, use it! */
      gnutls_session_set_data(session, ssl_sessionid, ssl_idsize);

      /* Informational message */
      infof(data, "SSL re-using session ID\n");
    }
    Curl_ssl_sessionid_unlock(data);
  }

  return CURLE_OK;
}