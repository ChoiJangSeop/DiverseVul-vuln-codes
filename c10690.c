AvahiSHostNameResolver *avahi_s_host_name_resolver_new(
    AvahiServer *server,
    AvahiIfIndex interface,
    AvahiProtocol protocol,
    const char *host_name,
    AvahiProtocol aprotocol,
    AvahiLookupFlags flags,
    AvahiSHostNameResolverCallback callback,
    void* userdata) {
        AvahiSHostNameResolver *b;

        b = avahi_s_host_name_resolver_prepare(server, interface, protocol, host_name, aprotocol, flags, callback, userdata);
        avahi_s_host_name_resolver_start(b);

        return b;
}