static bool handle(app_data_t* app, pn_event_t* event) {
  switch (pn_event_type(event)) {

   case PN_CONNECTION_INIT: {
     pn_connection_t* c = pn_event_connection(event);
     pn_session_t* s = pn_session(pn_event_connection(event));
     pn_connection_set_container(c, app->container_id);
     pn_connection_open(c);
     pn_session_open(s);
     {
     pn_link_t* l = pn_sender(s, "my_sender");
     pn_terminus_set_address(pn_link_target(l), app->amqp_address);
     pn_link_open(l);
     break;
     }
   }

   case PN_CONNECTION_REMOTE_OPEN: {
     pn_ssl_t *ssl = pn_ssl(pn_event_transport(event));
     if (ssl) {
       char name[1024];
       pn_ssl_get_protocol_name(ssl, name, sizeof(name));
       printf("secure connection: %s\n", name);
       fflush(stdout);
     }
     break;
   }

   case PN_LINK_FLOW: {
     /* The peer has given us some credit, now we can send messages */
     pn_link_t *sender = pn_event_link(event);
     while (pn_link_credit(sender) > 0 && app->sent < app->message_count) {
       ++app->sent;
       /* Use sent counter as unique delivery tag. */
       pn_delivery(sender, pn_dtag((const char *)&app->sent, sizeof(app->sent)));
       {
       pn_bytes_t msgbuf = encode_message(app);
       pn_link_send(sender, msgbuf.start, msgbuf.size);
       }
       pn_link_advance(sender);
     }
     break;
   }

   case PN_DELIVERY: {
     /* We received acknowledgement from the peer that a message was delivered. */
     pn_delivery_t* d = pn_event_delivery(event);
     if (pn_delivery_remote_state(d) == PN_ACCEPTED) {
       if (++app->acknowledged == app->message_count) {
         printf("%d messages sent and acknowledged\n", app->acknowledged);
         fflush(stdout);
         pn_connection_close(pn_event_connection(event));
         /* Continue handling events till we receive TRANSPORT_CLOSED */
       }
     } else {
       fprintf(stderr, "unexpected delivery state %d\n", (int)pn_delivery_remote_state(d));
       pn_connection_close(pn_event_connection(event));
       exit_code=1;
     }
     break;
   }

   case PN_TRANSPORT_CLOSED:
    check_condition(event, pn_transport_condition(pn_event_transport(event)));
    break;

   case PN_CONNECTION_REMOTE_CLOSE:
    check_condition(event, pn_connection_remote_condition(pn_event_connection(event)));
    pn_connection_close(pn_event_connection(event));
    break;

   case PN_SESSION_REMOTE_CLOSE:
    check_condition(event, pn_session_remote_condition(pn_event_session(event)));
    pn_connection_close(pn_event_connection(event));
    break;

   case PN_LINK_REMOTE_CLOSE:
   case PN_LINK_REMOTE_DETACH:
    check_condition(event, pn_link_remote_condition(pn_event_link(event)));
    pn_connection_close(pn_event_connection(event));
    break;

   case PN_PROACTOR_INACTIVE:
    return false;

   default: break;
  }
  return true;
}