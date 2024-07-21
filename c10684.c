AvahiSDomainBrowser *avahi_s_domain_browser_new(
    AvahiServer *server,
    AvahiIfIndex interface,
    AvahiProtocol protocol,
    const char *domain,
    AvahiDomainBrowserType type,
    AvahiLookupFlags flags,
    AvahiSDomainBrowserCallback callback,
    void* userdata) {
        AvahiSDomainBrowser *b;

        b = avahi_s_domain_browser_prepare(server, interface, protocol, domain, type, flags, callback, userdata);
        avahi_s_domain_browser_start(b);

        return b;
}