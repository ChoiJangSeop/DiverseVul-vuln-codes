AvahiSDNSServerBrowser *avahi_s_dns_server_browser_new(
    AvahiServer *server,
    AvahiIfIndex interface,
    AvahiProtocol protocol,
    const char *domain,
    AvahiDNSServerType type,
    AvahiProtocol aprotocol,
    AvahiLookupFlags flags,
    AvahiSDNSServerBrowserCallback callback,
    void* userdata) {
        AvahiSDNSServerBrowser* b;

        b = avahi_s_dns_server_browser_prepare(server, interface, protocol, domain, type, aprotocol, flags, callback, userdata);
        avahi_s_dns_server_browser_start(b);

        return b;
}