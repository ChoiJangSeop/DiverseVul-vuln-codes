static int dns_transaction_emit_tcp(DnsTransaction *t) {
        _cleanup_close_ int fd = -1;
        _cleanup_(dns_stream_unrefp) DnsStream *s = NULL;
        union sockaddr_union sa;
        int r;

        assert(t);

        dns_transaction_close_connection(t);

        switch (t->scope->protocol) {

        case DNS_PROTOCOL_DNS:
                r = dns_transaction_pick_server(t);
                if (r < 0)
                        return r;

                if (!dns_server_dnssec_supported(t->server) && dns_type_is_dnssec(t->key->type))
                        return -EOPNOTSUPP;

                r = dns_server_adjust_opt(t->server, t->sent, t->current_feature_level);
                if (r < 0)
                        return r;

                if (t->server->stream && (DNS_SERVER_FEATURE_LEVEL_IS_TLS(t->current_feature_level) == t->server->stream->encrypted))
                        s = dns_stream_ref(t->server->stream);
                else
                        fd = dns_scope_socket_tcp(t->scope, AF_UNSPEC, NULL, t->server, dns_port_for_feature_level(t->current_feature_level), &sa);

                break;

        case DNS_PROTOCOL_LLMNR:
                /* When we already received a reply to this (but it was truncated), send to its sender address */
                if (t->received)
                        fd = dns_scope_socket_tcp(t->scope, t->received->family, &t->received->sender, NULL, t->received->sender_port, &sa);
                else {
                        union in_addr_union address;
                        int family = AF_UNSPEC;

                        /* Otherwise, try to talk to the owner of a
                         * the IP address, in case this is a reverse
                         * PTR lookup */

                        r = dns_name_address(dns_resource_key_name(t->key), &family, &address);
                        if (r < 0)
                                return r;
                        if (r == 0)
                                return -EINVAL;
                        if (family != t->scope->family)
                                return -ESRCH;

                        fd = dns_scope_socket_tcp(t->scope, family, &address, NULL, LLMNR_PORT, &sa);
                }

                break;

        default:
                return -EAFNOSUPPORT;
        }

        if (!s) {
                if (fd < 0)
                        return fd;

                r = dns_stream_new(t->scope->manager, &s, t->scope->protocol, fd, &sa);
                if (r < 0)
                        return r;

                fd = -1;

#if ENABLE_DNS_OVER_TLS
                if (t->scope->protocol == DNS_PROTOCOL_DNS &&
                    DNS_SERVER_FEATURE_LEVEL_IS_TLS(t->current_feature_level)) {

                        assert(t->server);
                        r = dnstls_stream_connect_tls(s, t->server);
                        if (r < 0)
                                return r;
                }
#endif

                if (t->server) {
                        dns_stream_unref(t->server->stream);
                        t->server->stream = dns_stream_ref(s);
                        s->server = dns_server_ref(t->server);
                }

                s->complete = on_stream_complete;
                s->on_packet = on_stream_packet;

                /* The interface index is difficult to determine if we are
                 * connecting to the local host, hence fill this in right away
                 * instead of determining it from the socket */
                s->ifindex = dns_scope_ifindex(t->scope);
        }

        t->stream = TAKE_PTR(s);
        LIST_PREPEND(transactions_by_stream, t->stream->transactions, t);

        r = dns_stream_write_packet(t->stream, t->sent);
        if (r < 0) {
                dns_transaction_close_connection(t);
                return r;
        }

        dns_transaction_reset_answer(t);

        t->tried_stream = true;

        return 0;
}