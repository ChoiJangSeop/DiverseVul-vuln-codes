AvahiSServiceTypeBrowser *avahi_s_service_type_browser_new(
    AvahiServer *server,
    AvahiIfIndex interface,
    AvahiProtocol protocol,
    const char *domain,
    AvahiLookupFlags flags,
    AvahiSServiceTypeBrowserCallback callback,
    void* userdata) {
        AvahiSServiceTypeBrowser *b;

        b = avahi_s_service_type_browser_prepare(server, interface, protocol, domain, flags, callback, userdata);
        avahi_s_service_type_browser_start(b);

        return b;
}