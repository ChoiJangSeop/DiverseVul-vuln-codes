directory_handle_command_get(dir_connection_t *conn, const char *headers,
                             const char *body, size_t body_len)
{
  size_t dlen;
  char *url, *url_mem, *header;
  or_options_t *options = get_options();
  time_t if_modified_since = 0;
  int compressed;
  size_t url_len;

  /* We ignore the body of a GET request. */
  (void)body;
  (void)body_len;

  log_debug(LD_DIRSERV,"Received GET command.");

  conn->_base.state = DIR_CONN_STATE_SERVER_WRITING;

  if (parse_http_url(headers, &url) < 0) {
    write_http_status_line(conn, 400, "Bad request");
    return 0;
  }
  if ((header = http_get_header(headers, "If-Modified-Since: "))) {
    struct tm tm;
    if (parse_http_time(header, &tm) == 0) {
      if_modified_since = tor_timegm(&tm);
    }
    /* The correct behavior on a malformed If-Modified-Since header is to
     * act as if no If-Modified-Since header had been given. */
    tor_free(header);
  }
  log_debug(LD_DIRSERV,"rewritten url as '%s'.", url);

  url_mem = url;
  url_len = strlen(url);
  compressed = url_len > 2 && !strcmp(url+url_len-2, ".z");
  if (compressed) {
    url[url_len-2] = '\0';
    url_len -= 2;
  }

  if (!strcmp(url,"/tor/")) {
    const char *frontpage = get_dirportfrontpage();

    if (frontpage) {
      dlen = strlen(frontpage);
      /* Let's return a disclaimer page (users shouldn't use V1 anymore,
         and caches don't fetch '/', so this is safe). */

      /* [We don't check for write_bucket_low here, since we want to serve
       *  this page no matter what.] */
      note_request(url, dlen);
      write_http_response_header_impl(conn, dlen, "text/html", "identity",
                                      NULL, DIRPORTFRONTPAGE_CACHE_LIFETIME);
      connection_write_to_buf(frontpage, dlen, TO_CONN(conn));
      goto done;
    }
    /* if no disclaimer file, fall through and continue */
  }

  if (!strcmp(url,"/tor/") || !strcmp(url,"/tor/dir")) { /* v1 dir fetch */
    cached_dir_t *d = dirserv_get_directory();

    if (!d) {
      log_info(LD_DIRSERV,"Client asked for the mirrored directory, but we "
               "don't have a good one yet. Sending 503 Dir not available.");
      write_http_status_line(conn, 503, "Directory unavailable");
      goto done;
    }
    if (d->published < if_modified_since) {
      write_http_status_line(conn, 304, "Not modified");
      goto done;
    }

    dlen = compressed ? d->dir_z_len : d->dir_len;

    if (global_write_bucket_low(TO_CONN(conn), dlen, 1)) {
      log_debug(LD_DIRSERV,
               "Client asked for the mirrored directory, but we've been "
               "writing too many bytes lately. Sending 503 Dir busy.");
      write_http_status_line(conn, 503, "Directory busy, try again later");
      goto done;
    }

    note_request(url, dlen);

    log_debug(LD_DIRSERV,"Dumping %sdirectory to client.",
              compressed?"compressed ":"");
    write_http_response_header(conn, dlen, compressed,
                          FULL_DIR_CACHE_LIFETIME);
    conn->cached_dir = d;
    conn->cached_dir_offset = 0;
    if (!compressed)
      conn->zlib_state = tor_zlib_new(0, ZLIB_METHOD);
    ++d->refcnt;

    /* Prime the connection with some data. */
    conn->dir_spool_src = DIR_SPOOL_CACHED_DIR;
    connection_dirserv_flushed_some(conn);
    goto done;
  }

  if (!strcmp(url,"/tor/running-routers")) { /* running-routers fetch */
    cached_dir_t *d = dirserv_get_runningrouters();
    if (!d) {
      write_http_status_line(conn, 503, "Directory unavailable");
      goto done;
    }
    if (d->published < if_modified_since) {
      write_http_status_line(conn, 304, "Not modified");
      goto done;
    }
    dlen = compressed ? d->dir_z_len : d->dir_len;

    if (global_write_bucket_low(TO_CONN(conn), dlen, 1)) {
      log_info(LD_DIRSERV,
               "Client asked for running-routers, but we've been "
               "writing too many bytes lately. Sending 503 Dir busy.");
      write_http_status_line(conn, 503, "Directory busy, try again later");
      goto done;
    }
    note_request(url, dlen);
    write_http_response_header(conn, dlen, compressed,
                 RUNNINGROUTERS_CACHE_LIFETIME);
    connection_write_to_buf(compressed ? d->dir_z : d->dir, dlen,
                            TO_CONN(conn));
    goto done;
  }

  if (!strcmpstart(url,"/tor/status/")
      || !strcmpstart(url, "/tor/status-vote/current/consensus")) {
    /* v2 or v3 network status fetch. */
    smartlist_t *dir_fps = smartlist_create();
    int is_v3 = !strcmpstart(url, "/tor/status-vote");
    geoip_client_action_t act =
        is_v3 ? GEOIP_CLIENT_NETWORKSTATUS : GEOIP_CLIENT_NETWORKSTATUS_V2;
    const char *request_type = NULL;
    const char *key = url + strlen("/tor/status/");
    long lifetime = NETWORKSTATUS_CACHE_LIFETIME;

    if (!is_v3) {
      dirserv_get_networkstatus_v2_fingerprints(dir_fps, key);
      if (!strcmpstart(key, "fp/"))
        request_type = compressed?"/tor/status/fp.z":"/tor/status/fp";
      else if (!strcmpstart(key, "authority"))
        request_type = compressed?"/tor/status/authority.z":
          "/tor/status/authority";
      else if (!strcmpstart(key, "all"))
        request_type = compressed?"/tor/status/all.z":"/tor/status/all";
      else
        request_type = "/tor/status/?";
    } else {
      networkstatus_t *v = networkstatus_get_latest_consensus();
      time_t now = time(NULL);
      const char *want_fps = NULL;
      char *flavor = NULL;
      #define CONSENSUS_URL_PREFIX "/tor/status-vote/current/consensus/"
      #define CONSENSUS_FLAVORED_PREFIX "/tor/status-vote/current/consensus-"
      /* figure out the flavor if any, and who we wanted to sign the thing */
      if (!strcmpstart(url, CONSENSUS_FLAVORED_PREFIX)) {
        const char *f, *cp;
        f = url + strlen(CONSENSUS_FLAVORED_PREFIX);
        cp = strchr(f, '/');
        if (cp) {
          want_fps = cp+1;
          flavor = tor_strndup(f, cp-f);
        } else {
          flavor = tor_strdup(f);
        }
      } else {
        if (!strcmpstart(url, CONSENSUS_URL_PREFIX))
          want_fps = url+strlen(CONSENSUS_URL_PREFIX);
      }

      /* XXXX MICRODESC NM NM should check document of correct flavor */
      if (v && want_fps &&
          !client_likes_consensus(v, want_fps)) {
        write_http_status_line(conn, 404, "Consensus not signed by sufficient "
                                          "number of requested authorities");
        smartlist_free(dir_fps);
        geoip_note_ns_response(act, GEOIP_REJECT_NOT_ENOUGH_SIGS);
        tor_free(flavor);
        goto done;
      }

      {
        char *fp = tor_malloc_zero(DIGEST_LEN);
        if (flavor)
          strlcpy(fp, flavor, DIGEST_LEN);
        tor_free(flavor);
        smartlist_add(dir_fps, fp);
      }
      request_type = compressed?"v3.z":"v3";
      lifetime = (v && v->fresh_until > now) ? v->fresh_until - now : 0;
    }

    if (!smartlist_len(dir_fps)) { /* we failed to create/cache cp */
      write_http_status_line(conn, 503, "Network status object unavailable");
      smartlist_free(dir_fps);
      geoip_note_ns_response(act, GEOIP_REJECT_UNAVAILABLE);
      goto done;
    }

    if (!dirserv_remove_old_statuses(dir_fps, if_modified_since)) {
      write_http_status_line(conn, 404, "Not found");
      SMARTLIST_FOREACH(dir_fps, char *, cp, tor_free(cp));
      smartlist_free(dir_fps);
      geoip_note_ns_response(act, GEOIP_REJECT_NOT_FOUND);
      goto done;
    } else if (!smartlist_len(dir_fps)) {
      write_http_status_line(conn, 304, "Not modified");
      SMARTLIST_FOREACH(dir_fps, char *, cp, tor_free(cp));
      smartlist_free(dir_fps);
      geoip_note_ns_response(act, GEOIP_REJECT_NOT_MODIFIED);
      goto done;
    }

    dlen = dirserv_estimate_data_size(dir_fps, 0, compressed);
    if (global_write_bucket_low(TO_CONN(conn), dlen, 2)) {
      log_debug(LD_DIRSERV,
               "Client asked for network status lists, but we've been "
               "writing too many bytes lately. Sending 503 Dir busy.");
      write_http_status_line(conn, 503, "Directory busy, try again later");
      SMARTLIST_FOREACH(dir_fps, char *, fp, tor_free(fp));
      smartlist_free(dir_fps);
      geoip_note_ns_response(act, GEOIP_REJECT_BUSY);
      goto done;
    }

    {
      struct in_addr in;
      if (tor_inet_aton((TO_CONN(conn))->address, &in)) {
        geoip_note_client_seen(act, ntohl(in.s_addr), time(NULL));
        geoip_note_ns_response(act, GEOIP_SUCCESS);
        /* Note that a request for a network status has started, so that we
         * can measure the download time later on. */
        if (TO_CONN(conn)->dirreq_id)
          geoip_start_dirreq(TO_CONN(conn)->dirreq_id, dlen, act,
                             DIRREQ_TUNNELED);
        else
          geoip_start_dirreq(TO_CONN(conn)->global_identifier, dlen, act,
                             DIRREQ_DIRECT);
      }
    }

    // note_request(request_type,dlen);
    (void) request_type;
    write_http_response_header(conn, -1, compressed,
                               smartlist_len(dir_fps) == 1 ? lifetime : 0);
    conn->fingerprint_stack = dir_fps;
    if (! compressed)
      conn->zlib_state = tor_zlib_new(0, ZLIB_METHOD);

    /* Prime the connection with some data. */
    conn->dir_spool_src = DIR_SPOOL_NETWORKSTATUS;
    connection_dirserv_flushed_some(conn);
    goto done;
  }

  if (!strcmpstart(url,"/tor/status-vote/current/") ||
      !strcmpstart(url,"/tor/status-vote/next/")) {
    /* XXXX If-modified-since is only implemented for the current
     * consensus: that's probably fine, since it's the only vote document
     * people fetch much. */
    int current;
    ssize_t body_len = 0;
    ssize_t estimated_len = 0;
    smartlist_t *items = smartlist_create();
    smartlist_t *dir_items = smartlist_create();
    int lifetime = 60; /* XXXX023 should actually use vote intervals. */
    url += strlen("/tor/status-vote/");
    current = !strcmpstart(url, "current/");
    url = strchr(url, '/');
    tor_assert(url);
    ++url;
    if (!strcmp(url, "consensus")) {
      const char *item;
      tor_assert(!current); /* we handle current consensus specially above,
                             * since it wants to be spooled. */
      if ((item = dirvote_get_pending_consensus(FLAV_NS)))
        smartlist_add(items, (char*)item);
    } else if (!current && !strcmp(url, "consensus-signatures")) {
      /* XXXX the spec says that we should implement
       * current/consensus-signatures too.  It doesn't seem to be needed,
       * though. */
      const char *item;
      if ((item=dirvote_get_pending_detached_signatures()))
        smartlist_add(items, (char*)item);
    } else if (!strcmp(url, "authority")) {
      const cached_dir_t *d;
      int flags = DGV_BY_ID |
        (current ? DGV_INCLUDE_PREVIOUS : DGV_INCLUDE_PENDING);
      if ((d=dirvote_get_vote(NULL, flags)))
        smartlist_add(dir_items, (cached_dir_t*)d);
    } else {
      const cached_dir_t *d;
      smartlist_t *fps = smartlist_create();
      int flags;
      if (!strcmpstart(url, "d/")) {
        url += 2;
        flags = DGV_INCLUDE_PENDING | DGV_INCLUDE_PREVIOUS;
      } else {
        flags = DGV_BY_ID |
          (current ? DGV_INCLUDE_PREVIOUS : DGV_INCLUDE_PENDING);
      }
      dir_split_resource_into_fingerprints(url, fps, NULL,
                                           DSR_HEX|DSR_SORT_UNIQ);
      SMARTLIST_FOREACH(fps, char *, fp, {
          if ((d = dirvote_get_vote(fp, flags)))
            smartlist_add(dir_items, (cached_dir_t*)d);
          tor_free(fp);
        });
      smartlist_free(fps);
    }
    if (!smartlist_len(dir_items) && !smartlist_len(items)) {
      write_http_status_line(conn, 404, "Not found");
      goto vote_done;
    }
    SMARTLIST_FOREACH(dir_items, cached_dir_t *, d,
                      body_len += compressed ? d->dir_z_len : d->dir_len);
    estimated_len += body_len;
    SMARTLIST_FOREACH(items, const char *, item, {
        size_t ln = strlen(item);
        if (compressed) {
          estimated_len += ln/2;
        } else {
          body_len += ln; estimated_len += ln;
        }
      });

    if (global_write_bucket_low(TO_CONN(conn), estimated_len, 2)) {
      write_http_status_line(conn, 503, "Directory busy, try again later.");
      goto vote_done;
    }
    write_http_response_header(conn, body_len ? body_len : -1, compressed,
                 lifetime);

    if (smartlist_len(items)) {
      if (compressed) {
        conn->zlib_state = tor_zlib_new(1, ZLIB_METHOD);
        SMARTLIST_FOREACH(items, const char *, c,
                 connection_write_to_buf_zlib(c, strlen(c), conn, 0));
        connection_write_to_buf_zlib("", 0, conn, 1);
      } else {
        SMARTLIST_FOREACH(items, const char *, c,
                         connection_write_to_buf(c, strlen(c), TO_CONN(conn)));
      }
    } else {
      SMARTLIST_FOREACH(dir_items, cached_dir_t *, d,
          connection_write_to_buf(compressed ? d->dir_z : d->dir,
                                  compressed ? d->dir_z_len : d->dir_len,
                                  TO_CONN(conn)));
    }
  vote_done:
    smartlist_free(items);
    smartlist_free(dir_items);
    goto done;
  }

  if (!strcmpstart(url, "/tor/micro/d/")) {
    smartlist_t *fps = smartlist_create();

    dir_split_resource_into_fingerprints(url+strlen("/tor/micro/d/"),
                                      fps, NULL,
                                      DSR_DIGEST256|DSR_BASE64|DSR_SORT_UNIQ);

    if (!dirserv_have_any_microdesc(fps)) {
      write_http_status_line(conn, 404, "Not found");
      SMARTLIST_FOREACH(fps, char *, fp, tor_free(fp));
      smartlist_free(fps);
      goto done;
    }
    dlen = dirserv_estimate_microdesc_size(fps, compressed);
    if (global_write_bucket_low(TO_CONN(conn), dlen, 2)) {
      log_info(LD_DIRSERV,
               "Client asked for server descriptors, but we've been "
               "writing too many bytes lately. Sending 503 Dir busy.");
      write_http_status_line(conn, 503, "Directory busy, try again later");
      SMARTLIST_FOREACH(fps, char *, fp, tor_free(fp));
      smartlist_free(fps);
      goto done;
    }

    write_http_response_header(conn, -1, compressed, MICRODESC_CACHE_LIFETIME);
    conn->dir_spool_src = DIR_SPOOL_MICRODESC;
    conn->fingerprint_stack = fps;

    if (compressed)
      conn->zlib_state = tor_zlib_new(1, ZLIB_METHOD);

    connection_dirserv_flushed_some(conn);
    goto done;
  }

  if (!strcmpstart(url,"/tor/server/") ||
      (!options->BridgeAuthoritativeDir &&
       !options->BridgeRelay && !strcmpstart(url,"/tor/extra/"))) {
    int res;
    const char *msg;
    const char *request_type = NULL;
    int cache_lifetime = 0;
    int is_extra = !strcmpstart(url,"/tor/extra/");
    url += is_extra ? strlen("/tor/extra/") : strlen("/tor/server/");
    conn->fingerprint_stack = smartlist_create();
    res = dirserv_get_routerdesc_fingerprints(conn->fingerprint_stack, url,
                                          &msg,
                                          !connection_dir_is_encrypted(conn),
                                          is_extra);

    if (!strcmpstart(url, "fp/")) {
      request_type = compressed?"/tor/server/fp.z":"/tor/server/fp";
      if (smartlist_len(conn->fingerprint_stack) == 1)
        cache_lifetime = ROUTERDESC_CACHE_LIFETIME;
    } else if (!strcmpstart(url, "authority")) {
      request_type = compressed?"/tor/server/authority.z":
        "/tor/server/authority";
      cache_lifetime = ROUTERDESC_CACHE_LIFETIME;
    } else if (!strcmpstart(url, "all")) {
      request_type = compressed?"/tor/server/all.z":"/tor/server/all";
      cache_lifetime = FULL_DIR_CACHE_LIFETIME;
    } else if (!strcmpstart(url, "d/")) {
      request_type = compressed?"/tor/server/d.z":"/tor/server/d";
      if (smartlist_len(conn->fingerprint_stack) == 1)
        cache_lifetime = ROUTERDESC_BY_DIGEST_CACHE_LIFETIME;
    } else {
      request_type = "/tor/server/?";
    }
    (void) request_type; /* usable for note_request. */
    if (!strcmpstart(url, "d/"))
      conn->dir_spool_src =
        is_extra ? DIR_SPOOL_EXTRA_BY_DIGEST : DIR_SPOOL_SERVER_BY_DIGEST;
    else
      conn->dir_spool_src =
        is_extra ? DIR_SPOOL_EXTRA_BY_FP : DIR_SPOOL_SERVER_BY_FP;

    if (!dirserv_have_any_serverdesc(conn->fingerprint_stack,
                                     conn->dir_spool_src)) {
      res = -1;
      msg = "Not found";
    }

    if (res < 0)
      write_http_status_line(conn, 404, msg);
    else {
      dlen = dirserv_estimate_data_size(conn->fingerprint_stack,
                                        1, compressed);
      if (global_write_bucket_low(TO_CONN(conn), dlen, 2)) {
        log_info(LD_DIRSERV,
                 "Client asked for server descriptors, but we've been "
                 "writing too many bytes lately. Sending 503 Dir busy.");
        write_http_status_line(conn, 503, "Directory busy, try again later");
        conn->dir_spool_src = DIR_SPOOL_NONE;
        goto done;
      }
      write_http_response_header(conn, -1, compressed, cache_lifetime);
      if (compressed)
        conn->zlib_state = tor_zlib_new(1, ZLIB_METHOD);
      /* Prime the connection with some data. */
      connection_dirserv_flushed_some(conn);
    }
    goto done;
  }

  if (!strcmpstart(url,"/tor/keys/")) {
    smartlist_t *certs = smartlist_create();
    ssize_t len = -1;
    if (!strcmp(url, "/tor/keys/all")) {
      authority_cert_get_all(certs);
    } else if (!strcmp(url, "/tor/keys/authority")) {
      authority_cert_t *cert = get_my_v3_authority_cert();
      if (cert)
        smartlist_add(certs, cert);
    } else if (!strcmpstart(url, "/tor/keys/fp/")) {
      smartlist_t *fps = smartlist_create();
      dir_split_resource_into_fingerprints(url+strlen("/tor/keys/fp/"),
                                           fps, NULL,
                                           DSR_HEX|DSR_SORT_UNIQ);
      SMARTLIST_FOREACH(fps, char *, d, {
          authority_cert_t *c = authority_cert_get_newest_by_id(d);
          if (c) smartlist_add(certs, c);
          tor_free(d);
      });
      smartlist_free(fps);
    } else if (!strcmpstart(url, "/tor/keys/sk/")) {
      smartlist_t *fps = smartlist_create();
      dir_split_resource_into_fingerprints(url+strlen("/tor/keys/sk/"),
                                           fps, NULL,
                                           DSR_HEX|DSR_SORT_UNIQ);
      SMARTLIST_FOREACH(fps, char *, d, {
          authority_cert_t *c = authority_cert_get_by_sk_digest(d);
          if (c) smartlist_add(certs, c);
          tor_free(d);
      });
      smartlist_free(fps);
    } else if (!strcmpstart(url, "/tor/keys/fp-sk/")) {
      smartlist_t *fp_sks = smartlist_create();
      dir_split_resource_into_fingerprint_pairs(url+strlen("/tor/keys/fp-sk/"),
                                                fp_sks);
      SMARTLIST_FOREACH(fp_sks, fp_pair_t *, pair, {
          authority_cert_t *c = authority_cert_get_by_digests(pair->first,
                                                              pair->second);
          if (c) smartlist_add(certs, c);
          tor_free(pair);
      });
      smartlist_free(fp_sks);
    } else {
      write_http_status_line(conn, 400, "Bad request");
      goto keys_done;
    }
    if (!smartlist_len(certs)) {
      write_http_status_line(conn, 404, "Not found");
      goto keys_done;
    }
    SMARTLIST_FOREACH(certs, authority_cert_t *, c,
      if (c->cache_info.published_on < if_modified_since)
        SMARTLIST_DEL_CURRENT(certs, c));
    if (!smartlist_len(certs)) {
      write_http_status_line(conn, 304, "Not modified");
      goto keys_done;
    }
    len = 0;
    SMARTLIST_FOREACH(certs, authority_cert_t *, c,
                      len += c->cache_info.signed_descriptor_len);

    if (global_write_bucket_low(TO_CONN(conn), compressed?len/2:len, 2)) {
      write_http_status_line(conn, 503, "Directory busy, try again later.");
      goto keys_done;
    }

    write_http_response_header(conn, compressed?-1:len, compressed, 60*60);
    if (compressed) {
      conn->zlib_state = tor_zlib_new(1, ZLIB_METHOD);
      SMARTLIST_FOREACH(certs, authority_cert_t *, c,
            connection_write_to_buf_zlib(c->cache_info.signed_descriptor_body,
                                         c->cache_info.signed_descriptor_len,
                                         conn, 0));
      connection_write_to_buf_zlib("", 0, conn, 1);
    } else {
      SMARTLIST_FOREACH(certs, authority_cert_t *, c,
            connection_write_to_buf(c->cache_info.signed_descriptor_body,
                                    c->cache_info.signed_descriptor_len,
                                    TO_CONN(conn)));
    }
  keys_done:
    smartlist_free(certs);
    goto done;
  }

  if (options->HidServDirectoryV2 &&
       !strcmpstart(url,"/tor/rendezvous2/")) {
    /* Handle v2 rendezvous descriptor fetch request. */
    const char *descp;
    const char *query = url + strlen("/tor/rendezvous2/");
    if (strlen(query) == REND_DESC_ID_V2_LEN_BASE32) {
      log_info(LD_REND, "Got a v2 rendezvous descriptor request for ID '%s'",
               safe_str(query));
      switch (rend_cache_lookup_v2_desc_as_dir(query, &descp)) {
        case 1: /* valid */
          write_http_response_header(conn, strlen(descp), 0, 0);
          connection_write_to_buf(descp, strlen(descp), TO_CONN(conn));
          break;
        case 0: /* well-formed but not present */
          write_http_status_line(conn, 404, "Not found");
          break;
        case -1: /* not well-formed */
          write_http_status_line(conn, 400, "Bad request");
          break;
      }
    } else { /* not well-formed */
      write_http_status_line(conn, 400, "Bad request");
    }
    goto done;
  }

  if (options->HSAuthoritativeDir && !strcmpstart(url,"/tor/rendezvous/")) {
    /* rendezvous descriptor fetch */
    const char *descp;
    size_t desc_len;
    const char *query = url+strlen("/tor/rendezvous/");

    log_info(LD_REND, "Handling rendezvous descriptor get");
    switch (rend_cache_lookup_desc(query, 0, &descp, &desc_len)) {
      case 1: /* valid */
        write_http_response_header_impl(conn, desc_len,
                                        "application/octet-stream",
                                        NULL, NULL, 0);
        note_request("/tor/rendezvous?/", desc_len);
        /* need to send descp separately, because it may include NULs */
        connection_write_to_buf(descp, desc_len, TO_CONN(conn));
        break;
      case 0: /* well-formed but not present */
        write_http_status_line(conn, 404, "Not found");
        break;
      case -1: /* not well-formed */
        write_http_status_line(conn, 400, "Bad request");
        break;
    }
    goto done;
  }

  if (options->BridgeAuthoritativeDir &&
      options->_BridgePassword_AuthDigest &&
      connection_dir_is_encrypted(conn) &&
      !strcmp(url,"/tor/networkstatus-bridges")) {
    char *status;
    char digest[DIGEST256_LEN];

    header = http_get_header(headers, "Authorization: Basic ");
    if (header)
      crypto_digest256(digest, header, strlen(header), DIGEST_SHA256);

    /* now make sure the password is there and right */
    if (!header ||
        tor_memneq(digest,
                   options->_BridgePassword_AuthDigest, DIGEST256_LEN)) {
      write_http_status_line(conn, 404, "Not found");
      tor_free(header);
      goto done;
    }
    tor_free(header);

    /* all happy now. send an answer. */
    status = networkstatus_getinfo_by_purpose("bridge", time(NULL));
    dlen = strlen(status);
    write_http_response_header(conn, dlen, 0, 0);
    connection_write_to_buf(status, dlen, TO_CONN(conn));
    tor_free(status);
    goto done;
  }

  if (!strcmpstart(url,"/tor/bytes.txt")) {
    char *bytes = directory_dump_request_log();
    size_t len = strlen(bytes);
    write_http_response_header(conn, len, 0, 0);
    connection_write_to_buf(bytes, len, TO_CONN(conn));
    tor_free(bytes);
    goto done;
  }

  if (!strcmp(url,"/tor/robots.txt")) { /* /robots.txt will have been
                                           rewritten to /tor/robots.txt */
    char robots[] = "User-agent: *\r\nDisallow: /\r\n";
    size_t len = strlen(robots);
    write_http_response_header(conn, len, 0, ROBOTS_CACHE_LIFETIME);
    connection_write_to_buf(robots, len, TO_CONN(conn));
    goto done;
  }

  if (!strcmp(url,"/tor/dbg-stability.txt")) {
    const char *stability;
    size_t len;
    if (options->BridgeAuthoritativeDir ||
        ! authdir_mode_tests_reachability(options) ||
        ! (stability = rep_hist_get_router_stability_doc(time(NULL)))) {
      write_http_status_line(conn, 404, "Not found.");
      goto done;
    }

    len = strlen(stability);
    write_http_response_header(conn, len, 0, 0);
    connection_write_to_buf(stability, len, TO_CONN(conn));
    goto done;
  }

#if defined(EXPORTMALLINFO) && defined(HAVE_MALLOC_H) && defined(HAVE_MALLINFO)
#define ADD_MALLINFO_LINE(x) do {                               \
    tor_snprintf(tmp, sizeof(tmp), "%s %d\n", #x, mi.x);        \
    smartlist_add(lines, tor_strdup(tmp));                      \
  }while(0);

  if (!strcmp(url,"/tor/mallinfo.txt") &&
      (tor_addr_eq_ipv4h(&conn->_base.addr, 0x7f000001ul))) {
    char *result;
    size_t len;
    struct mallinfo mi;
    smartlist_t *lines;
    char tmp[256];

    memset(&mi, 0, sizeof(mi));
    mi = mallinfo();
    lines = smartlist_create();

    ADD_MALLINFO_LINE(arena)
    ADD_MALLINFO_LINE(ordblks)
    ADD_MALLINFO_LINE(smblks)
    ADD_MALLINFO_LINE(hblks)
    ADD_MALLINFO_LINE(hblkhd)
    ADD_MALLINFO_LINE(usmblks)
    ADD_MALLINFO_LINE(fsmblks)
    ADD_MALLINFO_LINE(uordblks)
    ADD_MALLINFO_LINE(fordblks)
    ADD_MALLINFO_LINE(keepcost)

    result = smartlist_join_strings(lines, "", 0, NULL);
    SMARTLIST_FOREACH(lines, char *, cp, tor_free(cp));
    smartlist_free(lines);

    len = strlen(result);
    write_http_response_header(conn, len, 0, 0);
    connection_write_to_buf(result, len, TO_CONN(conn));
    tor_free(result);
    goto done;
  }
#endif

  /* we didn't recognize the url */
  write_http_status_line(conn, 404, "Not found");

 done:
  tor_free(url_mem);
  return 0;
}