AvahiSRecordBrowser *avahi_s_record_browser_new(
    AvahiServer *server,
    AvahiIfIndex interface,
    AvahiProtocol protocol,
    AvahiKey *key,
    AvahiLookupFlags flags,
    AvahiSRecordBrowserCallback callback,
    void* userdata) {
        AvahiSRecordBrowser *b;

        b = avahi_s_record_browser_prepare(server, interface, protocol, key, flags, callback, userdata);
        avahi_s_record_browser_start_query(b);

        return b;
}