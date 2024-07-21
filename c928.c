dns_resolve(edge_connection_t *exitconn)
{
  or_circuit_t *oncirc = TO_OR_CIRCUIT(exitconn->on_circuit);
  int is_resolve, r;
  char *hostname = NULL;
  is_resolve = exitconn->_base.purpose == EXIT_PURPOSE_RESOLVE;

  r = dns_resolve_impl(exitconn, is_resolve, oncirc, &hostname);

  switch (r) {
    case 1:
      /* We got an answer without a lookup -- either the answer was
       * cached, or it was obvious (like an IP address). */
      if (is_resolve) {
        /* Send the answer back right now, and detach. */
        if (hostname)
          send_resolved_hostname_cell(exitconn, hostname);
        else
          send_resolved_cell(exitconn, RESOLVED_TYPE_IPV4);
        exitconn->on_circuit = NULL;
      } else {
        /* Add to the n_streams list; the calling function will send back a
         * connected cell. */
        exitconn->next_stream = oncirc->n_streams;
        oncirc->n_streams = exitconn;
      }
      break;
    case 0:
      /* The request is pending: add the connection into the linked list of
       * resolving_streams on this circuit. */
      exitconn->_base.state = EXIT_CONN_STATE_RESOLVING;
      exitconn->next_stream = oncirc->resolving_streams;
      oncirc->resolving_streams = exitconn;
      break;
    case -2:
    case -1:
      /* The request failed before it could start: cancel this connection,
       * and stop everybody waiting for the same connection. */
      if (is_resolve) {
        send_resolved_cell(exitconn,
             (r == -1) ? RESOLVED_TYPE_ERROR : RESOLVED_TYPE_ERROR_TRANSIENT);
      }

      exitconn->on_circuit = NULL;

      dns_cancel_pending_resolve(exitconn->_base.address);

      if (!exitconn->_base.marked_for_close) {
        connection_free(TO_CONN(exitconn));
        // XXX ... and we just leak exitconn otherwise? -RD
        // If it's marked for close, it's on closeable_connection_lst in
        // main.c.  If it's on the closeable list, it will get freed from
        // main.c. -NM
        // "<armadev> If that's true, there are other bugs around, where we
        //  don't check if it's marked, and will end up double-freeing."
        // On the other hand, I don't know of any actual bugs here, so this
        // shouldn't be holding up the rc. -RD
      }
      break;
    default:
      tor_assert(0);
  }

  tor_free(hostname);
  return r;
}