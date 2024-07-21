static int originates_from_local_legacy_unicast_socket(AvahiServer *s, const AvahiAddress *address, uint16_t port) {
    assert(s);
    assert(address);
    assert(port > 0);

    if (!s->config.enable_reflector)
        return 0;

    if (!avahi_address_is_local(s->monitor, address))
        return 0;

    if (address->proto == AVAHI_PROTO_INET && s->fd_legacy_unicast_ipv4 >= 0) {
        struct sockaddr_in lsa;
        socklen_t l = sizeof(lsa);

        if (getsockname(s->fd_legacy_unicast_ipv4, (struct sockaddr*) &lsa, &l) != 0)
            avahi_log_warn("getsockname(): %s", strerror(errno));
        else
            return lsa.sin_port == port;

    }

    if (address->proto == AVAHI_PROTO_INET6 && s->fd_legacy_unicast_ipv6 >= 0) {
        struct sockaddr_in6 lsa;
        socklen_t l = sizeof(lsa);

        if (getsockname(s->fd_legacy_unicast_ipv6, (struct sockaddr*) &lsa, &l) != 0)
            avahi_log_warn("getsockname(): %s", strerror(errno));
        else
            return lsa.sin6_port == port;
    }

    return 0;
}