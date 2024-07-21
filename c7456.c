
static bool handle(broker_t* b, pn_event_t* e) {
  pn_connection_t *c = pn_event_connection(e);

  switch (pn_event_type(e)) {

   case PN_LISTENER_OPEN: {
     char port[PN_MAX_ADDR];    /* Get the listening port */
     pn_netaddr_host_port(pn_listener_addr(pn_event_listener(e)), NULL, 0, port, sizeof(port));
     printf("listening on %s\n", port);
     fflush(stdout);
     break;
   }
   case PN_LISTENER_ACCEPT: {
    /* Configure a transport to allow SSL and SASL connections. See ssl_domain setup in main() */
     pn_transport_t *t = pn_transport();
     pn_transport_set_server(t); /* Must call before pn_sasl() */
     pn_sasl_allowed_mechs(pn_sasl(t), "ANONYMOUS");
     if (b->ssl_domain) {
       pn_ssl_init(pn_ssl(t), b->ssl_domain, NULL);
     }
     pn_listener_accept2(pn_event_listener(e), NULL, t);
     break;
   }
   case PN_CONNECTION_INIT:
     pn_connection_set_container(c, b->container_id);
     break;

   case PN_CONNECTION_REMOTE_OPEN: {
     pn_connection_open(pn_event_connection(e)); /* Complete the open */
     break;
   }
   case PN_CONNECTION_WAKE: {
     if (get_check_queues(c)) {
       int flags = PN_LOCAL_ACTIVE&PN_REMOTE_ACTIVE;
       pn_link_t *l;
       set_check_queues(c, false);
       for (l = pn_link_head(c, flags); l != NULL; l = pn_link_next(l, flags))
         link_send(b, l);
     }
     break;
   }
   case PN_SESSION_REMOTE_OPEN: {
     pn_session_open(pn_event_session(e));
     break;
   }
   case PN_LINK_REMOTE_OPEN: {
     pn_link_t *l = pn_event_link(e);
     if (pn_link_is_sender(l)) {
       const char *source = pn_terminus_get_address(pn_link_remote_source(l));
       pn_terminus_set_address(pn_link_source(l), source);
     } else {
       const char* target = pn_terminus_get_address(pn_link_remote_target(l));
       pn_terminus_set_address(pn_link_target(l), target);
       pn_link_flow(l, WINDOW);
     }
     pn_link_open(l);
     break;
   }
   case PN_LINK_FLOW: {
     link_send(b, pn_event_link(e));
     break;
   }
   case PN_LINK_FINAL: {
     pn_rwbytes_t *buf = (pn_rwbytes_t*)pn_link_get_context(pn_event_link(e));
     if (buf) {
       free(buf->start);
       free(buf);
     }
     break;
   }
   case PN_DELIVERY: {          /* Incoming message data */
     pn_delivery_t *d = pn_event_delivery(e);
     if (pn_delivery_readable(d)) {
       pn_link_t *l = pn_delivery_link(d);
       size_t size = pn_delivery_pending(d);
       pn_rwbytes_t* m = message_buffer(l); /* Append data to incoming message buffer */
       ssize_t recv;
       m->size += size;
       m->start = (char*)realloc(m->start, m->size);
       recv = pn_link_recv(l, m->start, m->size);
       if (recv == PN_ABORTED) { /*  */
         printf("Message aborted\n");
         fflush(stdout);
         m->size = 0;           /* Forget the data we accumulated */
         pn_delivery_settle(d); /* Free the delivery so we can receive the next message */
         pn_link_flow(l, WINDOW - pn_link_credit(l)); /* Replace credit for the aborted message */
       } else if (recv < 0 && recv != PN_EOS) {        /* Unexpected error */
           pn_condition_format(pn_link_condition(l), "broker", "PN_DELIVERY error: %s", pn_code((int)recv));
         pn_link_close(l);               /* Unexpected error, close the link */
       } else if (!pn_delivery_partial(d)) { /* Message is complete */
         const char *qname = pn_terminus_get_address(pn_link_target(l));
         queue_receive(b->proactor, queues_get(&b->queues, qname), *m);
         *m = pn_rwbytes_null;  /* Reset the buffer for the next message*/
         pn_delivery_update(d, PN_ACCEPTED);
         pn_delivery_settle(d);
         pn_link_flow(l, WINDOW - pn_link_credit(l));
       }
     }
     break;
   }

   case PN_TRANSPORT_CLOSED:
    check_condition(e, pn_transport_condition(pn_event_transport(e)));
    connection_unsub(b, pn_event_connection(e));
    break;

   case PN_CONNECTION_REMOTE_CLOSE:
    check_condition(e, pn_connection_remote_condition(pn_event_connection(e)));
    pn_connection_close(pn_event_connection(e));
    break;

   case PN_SESSION_REMOTE_CLOSE:
    check_condition(e, pn_session_remote_condition(pn_event_session(e)));
    session_unsub(b, pn_event_session(e));
    pn_session_close(pn_event_session(e));
    pn_session_free(pn_event_session(e));
    break;

   case PN_LINK_REMOTE_CLOSE:
    check_condition(e, pn_link_remote_condition(pn_event_link(e)));
    link_unsub(b, pn_event_link(e));
    pn_link_close(pn_event_link(e));
    pn_link_free(pn_event_link(e));
    break;

   case PN_LISTENER_CLOSE:
    check_condition(e, pn_listener_condition(pn_event_listener(e)));
    broker_stop(b);
    break;

    case PN_PROACTOR_INACTIVE:   /* listener and all connections closed */
    broker_stop(b);
    break;

   case PN_PROACTOR_INTERRUPT:
    pn_proactor_interrupt(b->proactor); /* Pass along the interrupt to the other threads */
    return false;

   default:
    break;
  }
  return true;