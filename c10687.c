AvahiSServiceResolver *avahi_s_service_resolver_new(
    AvahiServer *server,
    AvahiIfIndex interface,
    AvahiProtocol protocol,
    const char *name,
    const char *type,
    const char *domain,
    AvahiProtocol aprotocol,
    AvahiLookupFlags flags,
    AvahiSServiceResolverCallback callback,
    void* userdata) {
        AvahiSServiceResolver *b;

        b = avahi_s_service_resolver_prepare(server, interface, protocol, name, type, domain, aprotocol, flags, callback, userdata);
        avahi_s_service_resolver_start(b);

        return b;
}