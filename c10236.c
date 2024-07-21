static CURLcode bearssl_connect_step1(struct Curl_easy *data,
                                      struct connectdata *conn, int sockindex)
{
  struct ssl_connect_data *connssl = &conn->ssl[sockindex];
  struct ssl_backend_data *backend = connssl->backend;
  const char * const ssl_cafile = SSL_CONN_CONFIG(CAfile);
#ifndef CURL_DISABLE_PROXY
  const char *hostname = SSL_IS_PROXY() ? conn->http_proxy.host.name :
    conn->host.name;
#else
  const char *hostname = conn->host.name;
#endif
  const bool verifypeer = SSL_CONN_CONFIG(verifypeer);
  const bool verifyhost = SSL_CONN_CONFIG(verifyhost);
  CURLcode ret;
  unsigned version_min, version_max;
#ifdef ENABLE_IPV6
  struct in6_addr addr;
#else
  struct in_addr addr;
#endif

  switch(SSL_CONN_CONFIG(version)) {
  case CURL_SSLVERSION_SSLv2:
    failf(data, "BearSSL does not support SSLv2");
    return CURLE_SSL_CONNECT_ERROR;
  case CURL_SSLVERSION_SSLv3:
    failf(data, "BearSSL does not support SSLv3");
    return CURLE_SSL_CONNECT_ERROR;
  case CURL_SSLVERSION_TLSv1_0:
    version_min = BR_TLS10;
    version_max = BR_TLS10;
    break;
  case CURL_SSLVERSION_TLSv1_1:
    version_min = BR_TLS11;
    version_max = BR_TLS11;
    break;
  case CURL_SSLVERSION_TLSv1_2:
    version_min = BR_TLS12;
    version_max = BR_TLS12;
    break;
  case CURL_SSLVERSION_DEFAULT:
  case CURL_SSLVERSION_TLSv1:
    version_min = BR_TLS10;
    version_max = BR_TLS12;
    break;
  default:
    failf(data, "BearSSL: unknown CURLOPT_SSLVERSION");
    return CURLE_SSL_CONNECT_ERROR;
  }

  if(ssl_cafile) {
    ret = load_cafile(ssl_cafile, &backend->anchors, &backend->anchors_len);
    if(ret != CURLE_OK) {
      if(verifypeer) {
        failf(data, "error setting certificate verify locations."
              " CAfile: %s", ssl_cafile);
        return ret;
      }
      infof(data, "error setting certificate verify locations,"
            " continuing anyway:\n");
    }
  }

  /* initialize SSL context */
  br_ssl_client_init_full(&backend->ctx, &backend->x509.minimal,
                          backend->anchors, backend->anchors_len);
  br_ssl_engine_set_versions(&backend->ctx.eng, version_min, version_max);
  br_ssl_engine_set_buffer(&backend->ctx.eng, backend->buf,
                           sizeof(backend->buf), 1);

  /* initialize X.509 context */
  backend->x509.vtable = &x509_vtable;
  backend->x509.verifypeer = verifypeer;
  backend->x509.verifyhost = verifyhost;
  br_ssl_engine_set_x509(&backend->ctx.eng, &backend->x509.vtable);

  if(SSL_SET_OPTION(primary.sessionid)) {
    void *session;

    Curl_ssl_sessionid_lock(data);
    if(!Curl_ssl_getsessionid(data, conn, &session, NULL, sockindex)) {
      br_ssl_engine_set_session_parameters(&backend->ctx.eng, session);
      infof(data, "BearSSL: re-using session ID\n");
    }
    Curl_ssl_sessionid_unlock(data);
  }

  if(conn->bits.tls_enable_alpn) {
    int cur = 0;

    /* NOTE: when adding more protocols here, increase the size of the
     * protocols array in `struct ssl_backend_data`.
     */

#ifdef USE_NGHTTP2
    if(data->state.httpversion >= CURL_HTTP_VERSION_2
#ifndef CURL_DISABLE_PROXY
      && (!SSL_IS_PROXY() || !conn->bits.tunnel_proxy)
#endif
      ) {
      backend->protocols[cur++] = NGHTTP2_PROTO_VERSION_ID;
      infof(data, "ALPN, offering %s\n", NGHTTP2_PROTO_VERSION_ID);
    }
#endif

    backend->protocols[cur++] = ALPN_HTTP_1_1;
    infof(data, "ALPN, offering %s\n", ALPN_HTTP_1_1);

    br_ssl_engine_set_protocol_names(&backend->ctx.eng,
                                     backend->protocols, cur);
  }

  if((1 == Curl_inet_pton(AF_INET, hostname, &addr))
#ifdef ENABLE_IPV6
      || (1 == Curl_inet_pton(AF_INET6, hostname, &addr))
#endif
     ) {
    if(verifyhost) {
      failf(data, "BearSSL: "
            "host verification of IP address is not supported");
      return CURLE_PEER_FAILED_VERIFICATION;
    }
    hostname = NULL;
  }

  if(!br_ssl_client_reset(&backend->ctx, hostname, 0))
    return CURLE_FAILED_INIT;
  backend->active = TRUE;

  connssl->connecting_state = ssl_connect_2;

  return CURLE_OK;
}