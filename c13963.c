ConnectionExists(struct Curl_easy *data,
                 struct connectdata *needle,
                 struct connectdata **usethis,
                 bool *force_reuse,
                 bool *waitpipe)
{
  struct connectdata *check;
  struct connectdata *chosen = 0;
  bool foundPendingCandidate = FALSE;
  bool canmultiplex = IsMultiplexingPossible(data, needle);
  struct connectbundle *bundle;
  const char *hostbundle;

#ifdef USE_NTLM
  bool wantNTLMhttp = ((data->state.authhost.want &
                        (CURLAUTH_NTLM | CURLAUTH_NTLM_WB)) &&
                       (needle->handler->protocol & PROTO_FAMILY_HTTP));
#ifndef CURL_DISABLE_PROXY
  bool wantProxyNTLMhttp = (needle->bits.proxy_user_passwd &&
                            ((data->state.authproxy.want &
                              (CURLAUTH_NTLM | CURLAUTH_NTLM_WB)) &&
                             (needle->handler->protocol & PROTO_FAMILY_HTTP)));
#else
  bool wantProxyNTLMhttp = FALSE;
#endif
#endif

  *force_reuse = FALSE;
  *waitpipe = FALSE;

  /* Look up the bundle with all the connections to this particular host.
     Locks the connection cache, beware of early returns! */
  bundle = Curl_conncache_find_bundle(data, needle, data->state.conn_cache,
                                      &hostbundle);
  if(bundle) {
    /* Max pipe length is zero (unlimited) for multiplexed connections */
    struct Curl_llist_element *curr;

    infof(data, "Found bundle for host %s: %p [%s]",
          hostbundle, (void *)bundle, (bundle->multiuse == BUNDLE_MULTIPLEX ?
                                       "can multiplex" : "serially"));

    /* We can't multiplex if we don't know anything about the server */
    if(canmultiplex) {
      if(bundle->multiuse == BUNDLE_UNKNOWN) {
        if(data->set.pipewait) {
          infof(data, "Server doesn't support multiplex yet, wait");
          *waitpipe = TRUE;
          CONNCACHE_UNLOCK(data);
          return FALSE; /* no re-use */
        }

        infof(data, "Server doesn't support multiplex (yet)");
        canmultiplex = FALSE;
      }
      if((bundle->multiuse == BUNDLE_MULTIPLEX) &&
         !Curl_multiplex_wanted(data->multi)) {
        infof(data, "Could multiplex, but not asked to");
        canmultiplex = FALSE;
      }
      if(bundle->multiuse == BUNDLE_NO_MULTIUSE) {
        infof(data, "Can not multiplex, even if we wanted to");
        canmultiplex = FALSE;
      }
    }

    curr = bundle->conn_list.head;
    while(curr) {
      bool match = FALSE;
      size_t multiplexed = 0;

      /*
       * Note that if we use a HTTP proxy in normal mode (no tunneling), we
       * check connections to that proxy and not to the actual remote server.
       */
      check = curr->ptr;
      curr = curr->next;

      if(check->bits.connect_only || check->bits.close)
        /* connect-only or to-be-closed connections will not be reused */
        continue;

      if(extract_if_dead(check, data)) {
        /* disconnect it */
        Curl_disconnect(data, check, TRUE);
        continue;
      }

      if(data->set.ipver != CURL_IPRESOLVE_WHATEVER
          && data->set.ipver != check->ip_version) {
        /* skip because the connection is not via the requested IP version */
        continue;
      }

      if(bundle->multiuse == BUNDLE_MULTIPLEX)
        multiplexed = CONN_INUSE(check);

      if(!canmultiplex) {
        if(multiplexed) {
          /* can only happen within multi handles, and means that another easy
             handle is using this connection */
          continue;
        }

        if(Curl_resolver_asynch()) {
          /* primary_ip[0] is NUL only if the resolving of the name hasn't
             completed yet and until then we don't re-use this connection */
          if(!check->primary_ip[0]) {
            infof(data,
                  "Connection #%ld is still name resolving, can't reuse",
                  check->connection_id);
            continue;
          }
        }

        if(check->sock[FIRSTSOCKET] == CURL_SOCKET_BAD) {
          foundPendingCandidate = TRUE;
          /* Don't pick a connection that hasn't connected yet */
          infof(data, "Connection #%ld isn't open enough, can't reuse",
                check->connection_id);
          continue;
        }
      }

#ifdef USE_UNIX_SOCKETS
      if(needle->unix_domain_socket) {
        if(!check->unix_domain_socket)
          continue;
        if(strcmp(needle->unix_domain_socket, check->unix_domain_socket))
          continue;
        if(needle->bits.abstract_unix_socket !=
           check->bits.abstract_unix_socket)
          continue;
      }
      else if(check->unix_domain_socket)
        continue;
#endif

      if((needle->handler->flags&PROTOPT_SSL) !=
         (check->handler->flags&PROTOPT_SSL))
        /* don't do mixed SSL and non-SSL connections */
        if(get_protocol_family(check->handler) !=
           needle->handler->protocol || !check->bits.tls_upgraded)
          /* except protocols that have been upgraded via TLS */
          continue;

#ifndef CURL_DISABLE_PROXY
      if(needle->bits.httpproxy != check->bits.httpproxy ||
         needle->bits.socksproxy != check->bits.socksproxy)
        continue;

      if(needle->bits.socksproxy &&
        !socks_proxy_info_matches(&needle->socks_proxy,
                                  &check->socks_proxy))
        continue;
#endif
      if(needle->bits.conn_to_host != check->bits.conn_to_host)
        /* don't mix connections that use the "connect to host" feature and
         * connections that don't use this feature */
        continue;

      if(needle->bits.conn_to_port != check->bits.conn_to_port)
        /* don't mix connections that use the "connect to port" feature and
         * connections that don't use this feature */
        continue;

#ifndef CURL_DISABLE_PROXY
      if(needle->bits.httpproxy) {
        if(!proxy_info_matches(&needle->http_proxy, &check->http_proxy))
          continue;

        if(needle->bits.tunnel_proxy != check->bits.tunnel_proxy)
          continue;

        if(needle->http_proxy.proxytype == CURLPROXY_HTTPS) {
          /* use https proxy */
          if(needle->handler->flags&PROTOPT_SSL) {
            /* use double layer ssl */
            if(!Curl_ssl_config_matches(&needle->proxy_ssl_config,
                                        &check->proxy_ssl_config))
              continue;
            if(check->proxy_ssl[FIRSTSOCKET].state != ssl_connection_complete)
              continue;
          }

          if(!Curl_ssl_config_matches(&needle->ssl_config,
                                      &check->ssl_config))
            continue;
          if(check->ssl[FIRSTSOCKET].state != ssl_connection_complete)
            continue;
        }
      }
#endif

      if(!canmultiplex && CONN_INUSE(check))
        /* this request can't be multiplexed but the checked connection is
           already in use so we skip it */
        continue;

      if(CONN_INUSE(check)) {
        /* Subject for multiplex use if 'checks' belongs to the same multi
           handle as 'data' is. */
        struct Curl_llist_element *e = check->easyq.head;
        struct Curl_easy *entry = e->ptr;
        if(entry->multi != data->multi)
          continue;
      }

      if(needle->localdev || needle->localport) {
        /* If we are bound to a specific local end (IP+port), we must not
           re-use a random other one, although if we didn't ask for a
           particular one we can reuse one that was bound.

           This comparison is a bit rough and too strict. Since the input
           parameters can be specified in numerous ways and still end up the
           same it would take a lot of processing to make it really accurate.
           Instead, this matching will assume that re-uses of bound connections
           will most likely also re-use the exact same binding parameters and
           missing out a few edge cases shouldn't hurt anyone very much.
        */
        if((check->localport != needle->localport) ||
           (check->localportrange != needle->localportrange) ||
           (needle->localdev &&
            (!check->localdev || strcmp(check->localdev, needle->localdev))))
          continue;
      }

      if(!(needle->handler->flags & PROTOPT_CREDSPERREQUEST)) {
        /* This protocol requires credentials per connection,
           so verify that we're using the same name and password as well */
        if(strcmp(needle->user, check->user) ||
           strcmp(needle->passwd, check->passwd)) {
          /* one of them was different */
          continue;
        }
      }

      /* If multiplexing isn't enabled on the h2 connection and h1 is
         explicitly requested, handle it: */
      if((needle->handler->protocol & PROTO_FAMILY_HTTP) &&
         (check->httpversion >= 20) &&
         (data->state.httpwant < CURL_HTTP_VERSION_2_0))
        continue;

      if((needle->handler->flags&PROTOPT_SSL)
#ifndef CURL_DISABLE_PROXY
         || !needle->bits.httpproxy || needle->bits.tunnel_proxy
#endif
        ) {
        /* The requested connection does not use a HTTP proxy or it uses SSL or
           it is a non-SSL protocol tunneled or it is a non-SSL protocol which
           is allowed to be upgraded via TLS */

        if((strcasecompare(needle->handler->scheme, check->handler->scheme) ||
            (get_protocol_family(check->handler) ==
             needle->handler->protocol && check->bits.tls_upgraded)) &&
           (!needle->bits.conn_to_host || strcasecompare(
            needle->conn_to_host.name, check->conn_to_host.name)) &&
           (!needle->bits.conn_to_port ||
             needle->conn_to_port == check->conn_to_port) &&
           strcasecompare(needle->host.name, check->host.name) &&
           needle->remote_port == check->remote_port) {
          /* The schemes match or the protocol family is the same and the
             previous connection was TLS upgraded, and the hostname and host
             port match */
          if(needle->handler->flags & PROTOPT_SSL) {
            /* This is a SSL connection so verify that we're using the same
               SSL options as well */
            if(!Curl_ssl_config_matches(&needle->ssl_config,
                                        &check->ssl_config)) {
              DEBUGF(infof(data,
                           "Connection #%ld has different SSL parameters, "
                           "can't reuse",
                           check->connection_id));
              continue;
            }
            if(check->ssl[FIRSTSOCKET].state != ssl_connection_complete) {
              foundPendingCandidate = TRUE;
              DEBUGF(infof(data,
                           "Connection #%ld has not started SSL connect, "
                           "can't reuse",
                           check->connection_id));
              continue;
            }
          }
          match = TRUE;
        }
      }
      else {
        /* The requested connection is using the same HTTP proxy in normal
           mode (no tunneling) */
        match = TRUE;
      }

      if(match) {
#if defined(USE_NTLM)
        /* If we are looking for an HTTP+NTLM connection, check if this is
           already authenticating with the right credentials. If not, keep
           looking so that we can reuse NTLM connections if
           possible. (Especially we must not reuse the same connection if
           partway through a handshake!) */
        if(wantNTLMhttp) {
          if(strcmp(needle->user, check->user) ||
             strcmp(needle->passwd, check->passwd)) {

            /* we prefer a credential match, but this is at least a connection
               that can be reused and "upgraded" to NTLM */
            if(check->http_ntlm_state == NTLMSTATE_NONE)
              chosen = check;
            continue;
          }
        }
        else if(check->http_ntlm_state != NTLMSTATE_NONE) {
          /* Connection is using NTLM auth but we don't want NTLM */
          continue;
        }

#ifndef CURL_DISABLE_PROXY
        /* Same for Proxy NTLM authentication */
        if(wantProxyNTLMhttp) {
          /* Both check->http_proxy.user and check->http_proxy.passwd can be
           * NULL */
          if(!check->http_proxy.user || !check->http_proxy.passwd)
            continue;

          if(strcmp(needle->http_proxy.user, check->http_proxy.user) ||
             strcmp(needle->http_proxy.passwd, check->http_proxy.passwd))
            continue;
        }
        else if(check->proxy_ntlm_state != NTLMSTATE_NONE) {
          /* Proxy connection is using NTLM auth but we don't want NTLM */
          continue;
        }
#endif
        if(wantNTLMhttp || wantProxyNTLMhttp) {
          /* Credentials are already checked, we can use this connection */
          chosen = check;

          if((wantNTLMhttp &&
             (check->http_ntlm_state != NTLMSTATE_NONE)) ||
              (wantProxyNTLMhttp &&
               (check->proxy_ntlm_state != NTLMSTATE_NONE))) {
            /* We must use this connection, no other */
            *force_reuse = TRUE;
            break;
          }

          /* Continue look up for a better connection */
          continue;
        }
#endif
        if(canmultiplex) {
          /* We can multiplex if we want to. Let's continue looking for
             the optimal connection to use. */

          if(!multiplexed) {
            /* We have the optimal connection. Let's stop looking. */
            chosen = check;
            break;
          }

#ifdef USE_NGHTTP2
          /* If multiplexed, make sure we don't go over concurrency limit */
          if(check->bits.multiplex) {
            /* Multiplexed connections can only be HTTP/2 for now */
            struct http_conn *httpc = &check->proto.httpc;
            if(multiplexed >= httpc->settings.max_concurrent_streams) {
              infof(data, "MAX_CONCURRENT_STREAMS reached, skip (%zu)",
                    multiplexed);
              continue;
            }
            else if(multiplexed >=
                    Curl_multi_max_concurrent_streams(data->multi)) {
              infof(data, "client side MAX_CONCURRENT_STREAMS reached"
                    ", skip (%zu)",
                    multiplexed);
              continue;
            }
          }
#endif
          /* When not multiplexed, we have a match here! */
          chosen = check;
          infof(data, "Multiplexed connection found");
          break;
        }
        else {
          /* We have found a connection. Let's stop searching. */
          chosen = check;
          break;
        }
      }
    }
  }

  if(chosen) {
    /* mark it as used before releasing the lock */
    Curl_attach_connnection(data, chosen);
    CONNCACHE_UNLOCK(data);
    *usethis = chosen;
    return TRUE; /* yes, we found one to use! */
  }
  CONNCACHE_UNLOCK(data);

  if(foundPendingCandidate && data->set.pipewait) {
    infof(data,
          "Found pending candidate for reuse and CURLOPT_PIPEWAIT is set");
    *waitpipe = TRUE;
  }

  return FALSE; /* no matching connecting exists */
}