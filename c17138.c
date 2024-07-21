AP_DECLARE(void) ap_update_vhost_from_headers(request_rec *r)
{
    core_server_config *conf = ap_get_core_module_config(r->server->module_config);
    const char *host_header = apr_table_get(r->headers_in, "Host");
    int is_v6literal = 0;
    int have_hostname_from_url = 0;

    if (r->hostname) {
        /*
         * If there was a host part in the Request-URI, ignore the 'Host'
         * header.
         */
        have_hostname_from_url = 1;
        is_v6literal = fix_hostname(r, NULL, conf->http_conformance);
    }
    else if (host_header != NULL) {
        is_v6literal = fix_hostname(r, host_header, conf->http_conformance);
    }
    if (r->status != HTTP_OK)
        return;

    if (conf->http_conformance != AP_HTTP_CONFORMANCE_UNSAFE) {
        /*
         * If we have both hostname from an absoluteURI and a Host header,
         * we must ignore the Host header (RFC 2616 5.2).
         * To enforce this, we reset the Host header to the value from the
         * request line.
         */
        if (have_hostname_from_url && host_header != NULL) {
            const char *repl = construct_host_header(r, is_v6literal);
            apr_table_setn(r->headers_in, "Host", repl);
            ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, APLOGNO(02417)
                          "Replacing host header '%s' with host '%s' given "
                          "in the request uri", host_header, repl);
        }
    }

    /* check if we tucked away a name_chain */
    if (r->connection->vhost_lookup_data) {
        if (r->hostname)
            check_hostalias(r);
        else
            check_serverpath(r);
    }
}