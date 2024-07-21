AvahiSAddressResolver *avahi_s_address_resolver_new(
    AvahiServer *server,
    AvahiIfIndex interface,
    AvahiProtocol protocol,
    const AvahiAddress *address,
    AvahiLookupFlags flags,
    AvahiSAddressResolverCallback callback,
    void* userdata) {
        AvahiSAddressResolver *b;

        b = avahi_s_address_resolver_prepare(server, interface, protocol, address, flags, callback, userdata);
        avahi_s_address_resolver_start(b);

        return b;
}