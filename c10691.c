AvahiSServiceBrowser *avahi_s_service_browser_new(
    AvahiServer *server,
    AvahiIfIndex interface,
    AvahiProtocol protocol,
    const char *service_type,
    const char *domain,
    AvahiLookupFlags flags,
    AvahiSServiceBrowserCallback callback,
    void* userdata) {
        AvahiSServiceBrowser *b;

        b = avahi_s_service_browser_prepare(server, interface, protocol, service_type, domain, flags, callback, userdata);
        avahi_s_service_browser_start(b);

        return b;
}