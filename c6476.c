static int md_require_https_maybe(request_rec *r)
{
    const md_srv_conf_t *sc;
    apr_uri_t uri;
    const char *s;
    int status;
    
    if (opt_ssl_is_https 
        && strncmp(WELL_KNOWN_PREFIX, r->parsed_uri.path, sizeof(WELL_KNOWN_PREFIX)-1)) {
        
        sc = ap_get_module_config(r->server->module_config, &md_module);
        if (sc && sc->assigned && sc->assigned->require_https > MD_REQUIRE_OFF) {
            if (opt_ssl_is_https(r->connection)) {
                /* Using https:
                 * if 'permanent' and no one else set a HSTS header already, do it */
                if (sc->assigned->require_https == MD_REQUIRE_PERMANENT 
                    && sc->mc->hsts_header && !apr_table_get(r->headers_out, MD_HSTS_HEADER)) {
                    apr_table_setn(r->headers_out, MD_HSTS_HEADER, sc->mc->hsts_header);
                }
            }
            else {
                /* Not using https:, but require it. Redirect. */
                if (r->method_number == M_GET) {
                    /* safe to use the old-fashioned codes */
                    status = ((MD_REQUIRE_PERMANENT == sc->assigned->require_https)? 
                              HTTP_MOVED_PERMANENTLY : HTTP_MOVED_TEMPORARILY);
                }
                else {
                    /* these should keep the method unchanged on retry */
                    status = ((MD_REQUIRE_PERMANENT == sc->assigned->require_https)? 
                              HTTP_PERMANENT_REDIRECT : HTTP_TEMPORARY_REDIRECT);
                }
                
                s = ap_construct_url(r->pool, r->uri, r);
                if (APR_SUCCESS == apr_uri_parse(r->pool, s, &uri)) {
                    uri.scheme = (char*)"https";
                    uri.port = 443;
                    uri.port_str = (char*)"443";
                    uri.query = r->parsed_uri.query;
                    uri.fragment = r->parsed_uri.fragment;
                    s = apr_uri_unparse(r->pool, &uri, APR_URI_UNP_OMITUSERINFO);
                    if (s && *s) {
                        apr_table_setn(r->headers_out, "Location", s);
                        return status;
                    }
                }
            }
        }
    }
    return DECLINED;
}