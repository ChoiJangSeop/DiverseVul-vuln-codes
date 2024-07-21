SSLNetVConnection::sslStartHandShake(int event, int &err)
{
  if (sslHandshakeBeginTime == 0) {
    sslHandshakeBeginTime = Thread::get_hrtime();
    // net_activity will not be triggered until after the handshake
    set_inactivity_timeout(HRTIME_SECONDS(SSLConfigParams::ssl_handshake_timeout_in));
  }
  SSLConfig::scoped_config params;
  switch (event) {
  case SSL_EVENT_SERVER:
    if (this->ssl == nullptr) {
      SSLCertificateConfig::scoped_config lookup;
      IpEndpoint dst;
      int namelen = sizeof(dst);
      if (0 != safe_getsockname(this->get_socket(), &dst.sa, &namelen)) {
        Debug("ssl", "Failed to get dest ip, errno = [%d]", errno);
        return EVENT_ERROR;
      }
      SSLCertContext *cc = lookup->find(dst);
      if (is_debug_tag_set("ssl")) {
        IpEndpoint src;
        ip_port_text_buffer ipb1, ipb2;
        int ip_len = sizeof(src);

        if (0 != safe_getpeername(this->get_socket(), &src.sa, &ip_len)) {
          Debug("ssl", "Failed to get src ip, errno = [%d]", errno);
          return EVENT_ERROR;
        }
        ats_ip_nptop(&dst, ipb1, sizeof(ipb1));
        ats_ip_nptop(&src, ipb2, sizeof(ipb2));
        Debug("ssl", "IP context is %p for [%s] -> [%s], default context %p", cc, ipb2, ipb1, lookup->defaultContext());
      }

      // Escape if this is marked to be a tunnel.
      // No data has been read at this point, so we can go
      // directly into blind tunnel mode

      if (cc && SSLCertContext::OPT_TUNNEL == cc->opt) {
        if (this->is_transparent) {
          this->attributes     = HttpProxyPort::TRANSPORT_BLIND_TUNNEL;
          sslHandShakeComplete = true;
          SSL_free(this->ssl);
          this->ssl = nullptr;
          return EVENT_DONE;
        } else {
          SSLConfig::scoped_config params;
          this->SNIMapping = params->sni_map_enable;
          hookOpRequested  = SSL_HOOK_OP_TUNNEL;
        }
      }

      // Attach the default SSL_CTX to this SSL session. The default context is never going to be able
      // to negotiate a SSL session, but it's enough to trampoline us into the SNI callback where we
      // can select the right server certificate.
      this->ssl = make_ssl_connection(lookup->defaultContext(), this);
    }

    if (this->ssl == nullptr) {
      SSLErrorVC(this, "failed to create SSL server session");
      return EVENT_ERROR;
    }

    return sslServerHandShakeEvent(err);

  case SSL_EVENT_CLIENT:

    char buff[INET6_ADDRSTRLEN];

    if (this->ssl == nullptr) {
      // Making the check here instead of later, so we only
      // do this setting immediately after we create the SSL object
      SNIConfig::scoped_config sniParam;
      int8_t clientVerify = 0;
      cchar *serverKey    = this->options.sni_servername;
      if (!serverKey) {
        ats_ip_ntop(this->get_remote_addr(), buff, INET6_ADDRSTRLEN);
        serverKey = buff;
      }
      auto nps           = sniParam->getPropertyConfig(serverKey);
      SSL_CTX *clientCTX = nullptr;

      if (nps) {
        clientCTX    = nps->ctx;
        clientVerify = nps->verifyLevel;
      } else {
        clientCTX = params->client_ctx;
        // Keeping backwards compatability on the proxy.config.ssl.client.verify.server setting
        clientVerify = params->clientVerify ? (params->clientVerify == 1 ? 2 : 1) : 0;
      }
      if (!clientCTX) {
        SSLErrorVC(this, "failed to create SSL client session");
        return EVENT_ERROR;
      }

      this->ssl = make_ssl_connection(clientCTX, this);
      if (this->ssl == nullptr) {
        SSLErrorVC(this, "failed to create SSL client session");
        return EVENT_ERROR;
      }
      int verify_op;
      if (clientVerify) {
        verify_op = SSL_VERIFY_PEER;
        SSL_set_verify(this->ssl, verify_op, verify_callback);
      } else {
        // Don't bother to set the verify callback if no verification is required
        verify_op = SSL_VERIFY_NONE;
        SSL_set_verify(this->ssl, verify_op, nullptr);
      }

      if (this->options.sni_servername) {
        if (SSL_set_tlsext_host_name(this->ssl, this->options.sni_servername)) {
          Debug("ssl", "using SNI name '%s' for client handshake", this->options.sni_servername.get());
        } else {
          Debug("ssl.error", "failed to set SNI name '%s' for client handshake", this->options.sni_servername.get());
          SSL_INCREMENT_DYN_STAT(ssl_sni_name_set_failure);
        }
      }
    }

    return sslClientHandShakeEvent(err);

  default:
    ink_assert(0);
    return EVENT_ERROR;
  }
}