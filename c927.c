dns_resolve_impl(edge_connection_t *exitconn, int is_resolve,
                 or_circuit_t *oncirc, char **hostname_out)
{
  cached_resolve_t *resolve;
  cached_resolve_t search;
  pending_connection_t *pending_connection;
  const routerinfo_t *me;
  tor_addr_t addr;
  time_t now = time(NULL);
  uint8_t is_reverse = 0;
  int r;
  assert_connection_ok(TO_CONN(exitconn), 0);
  tor_assert(!SOCKET_OK(exitconn->_base.s));
  assert_cache_ok();
  tor_assert(oncirc);

  /* first check if exitconn->_base.address is an IP. If so, we already
   * know the answer. */
  if (tor_addr_parse(&addr, exitconn->_base.address) >= 0) {
    if (tor_addr_family(&addr) == AF_INET) {
      tor_addr_copy(&exitconn->_base.addr, &addr);
      exitconn->address_ttl = DEFAULT_DNS_TTL;
      return 1;
    } else {
      /* XXXX IPv6 */
      return -1;
    }
  }

  /* If we're a non-exit, don't even do DNS lookups. */
  if (!(me = router_get_my_routerinfo()) ||
      policy_is_reject_star(me->exit_policy)) {
    return -1;
  }
  if (address_is_invalid_destination(exitconn->_base.address, 0)) {
    log(LOG_PROTOCOL_WARN, LD_EXIT,
        "Rejecting invalid destination address %s",
        escaped_safe_str(exitconn->_base.address));
    return -1;
  }

  /* then take this opportunity to see if there are any expired
   * resolves in the hash table. */
  purge_expired_resolves(now);

  /* lower-case exitconn->_base.address, so it's in canonical form */
  tor_strlower(exitconn->_base.address);

  /* Check whether this is a reverse lookup.  If it's malformed, or it's a
   * .in-addr.arpa address but this isn't a resolve request, kill the
   * connection.
   */
  if ((r = tor_addr_parse_PTR_name(&addr, exitconn->_base.address,
                                              AF_UNSPEC, 0)) != 0) {
    if (r == 1) {
      is_reverse = 1;
      if (tor_addr_is_internal(&addr, 0)) /* internal address? */
        return -1;
    }

    if (!is_reverse || !is_resolve) {
      if (!is_reverse)
        log_info(LD_EXIT, "Bad .in-addr.arpa address \"%s\"; sending error.",
                 escaped_safe_str(exitconn->_base.address));
      else if (!is_resolve)
        log_info(LD_EXIT,
                 "Attempt to connect to a .in-addr.arpa address \"%s\"; "
                 "sending error.",
                 escaped_safe_str(exitconn->_base.address));

      return -1;
    }
    //log_notice(LD_EXIT, "Looks like an address %s",
    //exitconn->_base.address);
  }

  /* now check the hash table to see if 'address' is already there. */
  strlcpy(search.address, exitconn->_base.address, sizeof(search.address));
  resolve = HT_FIND(cache_map, &cache_root, &search);
  if (resolve && resolve->expire > now) { /* already there */
    switch (resolve->state) {
      case CACHE_STATE_PENDING:
        /* add us to the pending list */
        pending_connection = tor_malloc_zero(
                                      sizeof(pending_connection_t));
        pending_connection->conn = exitconn;
        pending_connection->next = resolve->pending_connections;
        resolve->pending_connections = pending_connection;
        log_debug(LD_EXIT,"Connection (fd %d) waiting for pending DNS "
                  "resolve of %s", exitconn->_base.s,
                  escaped_safe_str(exitconn->_base.address));
        return 0;
      case CACHE_STATE_CACHED_VALID:
        log_debug(LD_EXIT,"Connection (fd %d) found cached answer for %s",
                  exitconn->_base.s,
                  escaped_safe_str(resolve->address));
        exitconn->address_ttl = resolve->ttl;
        if (resolve->is_reverse) {
          tor_assert(is_resolve);
          *hostname_out = tor_strdup(resolve->result.hostname);
        } else {
          tor_addr_from_ipv4h(&exitconn->_base.addr, resolve->result.a.addr);
        }
        return 1;
      case CACHE_STATE_CACHED_FAILED:
        log_debug(LD_EXIT,"Connection (fd %d) found cached error for %s",
                  exitconn->_base.s,
                  escaped_safe_str(exitconn->_base.address));
        return -1;
      case CACHE_STATE_DONE:
        log_err(LD_BUG, "Found a 'DONE' dns resolve still in the cache.");
        tor_fragile_assert();
    }
    tor_assert(0);
  }
  tor_assert(!resolve);
  /* not there, need to add it */
  resolve = tor_malloc_zero(sizeof(cached_resolve_t));
  resolve->magic = CACHED_RESOLVE_MAGIC;
  resolve->state = CACHE_STATE_PENDING;
  resolve->minheap_idx = -1;
  resolve->is_reverse = is_reverse;
  strlcpy(resolve->address, exitconn->_base.address, sizeof(resolve->address));

  /* add this connection to the pending list */
  pending_connection = tor_malloc_zero(sizeof(pending_connection_t));
  pending_connection->conn = exitconn;
  resolve->pending_connections = pending_connection;

  /* Add this resolve to the cache and priority queue. */
  HT_INSERT(cache_map, &cache_root, resolve);
  set_expiry(resolve, now + RESOLVE_MAX_TIMEOUT);

  log_debug(LD_EXIT,"Launching %s.",
            escaped_safe_str(exitconn->_base.address));
  assert_cache_ok();

  return launch_resolve(exitconn);
}