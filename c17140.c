AP_CORE_DECLARE(void) ap_parse_uri(request_rec *r, const char *uri)
{
    int status = HTTP_OK;

    r->unparsed_uri = apr_pstrdup(r->pool, uri);

    /* http://issues.apache.org/bugzilla/show_bug.cgi?id=31875
     * http://issues.apache.org/bugzilla/show_bug.cgi?id=28450
     *
     * This is not in fact a URI, it's a path.  That matters in the
     * case of a leading double-slash.  We need to resolve the issue
     * by normalizing that out before treating it as a URI.
     */
    while ((uri[0] == '/') && (uri[1] == '/')) {
        ++uri ;
    }
    if (r->method_number == M_CONNECT) {
        status = apr_uri_parse_hostinfo(r->pool, uri, &r->parsed_uri);
    }
    else {
        status = apr_uri_parse(r->pool, uri, &r->parsed_uri);
    }

    if (status == APR_SUCCESS) {
        /* if it has a scheme we may need to do absoluteURI vhost stuff */
        if (r->parsed_uri.scheme
            && !ap_cstr_casecmp(r->parsed_uri.scheme, ap_http_scheme(r))) {
            r->hostname = r->parsed_uri.hostname;
        }
        else if (r->method_number == M_CONNECT) {
            r->hostname = r->parsed_uri.hostname;
        }

        r->args = r->parsed_uri.query;
        r->uri = r->parsed_uri.path ? r->parsed_uri.path
                 : apr_pstrdup(r->pool, "/");

#if defined(OS2) || defined(WIN32)
        /* Handle path translations for OS/2 and plug security hole.
         * This will prevent "http://www.wherever.com/..\..\/" from
         * returning a directory for the root drive.
         */
        {
            char *x;

            for (x = r->uri; (x = strchr(x, '\\')) != NULL; )
                *x = '/';
        }
#endif /* OS2 || WIN32 */
    }
    else {
        r->args = NULL;
        r->hostname = NULL;
        r->status = HTTP_BAD_REQUEST;             /* set error status */
        r->uri = apr_pstrdup(r->pool, uri);
    }
}