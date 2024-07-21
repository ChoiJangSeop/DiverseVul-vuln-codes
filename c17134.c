static int read_request_line(request_rec *r, apr_bucket_brigade *bb)
{
    enum {
        rrl_none, rrl_badmethod, rrl_badwhitespace, rrl_excesswhitespace,
        rrl_missinguri, rrl_baduri, rrl_badprotocol, rrl_trailingtext,
        rrl_badmethod09, rrl_reject09
    } deferred_error = rrl_none;
    char *ll;
    char *uri;
    apr_size_t len;
    int num_blank_lines = DEFAULT_LIMIT_BLANK_LINES;
    core_server_config *conf = ap_get_core_module_config(r->server->module_config);
    int strict = (conf->http_conformance != AP_HTTP_CONFORMANCE_UNSAFE);

    /* Read past empty lines until we get a real request line,
     * a read error, the connection closes (EOF), or we timeout.
     *
     * We skip empty lines because browsers have to tack a CRLF on to the end
     * of POSTs to support old CERN webservers.  But note that we may not
     * have flushed any previous response completely to the client yet.
     * We delay the flush as long as possible so that we can improve
     * performance for clients that are pipelining requests.  If a request
     * is pipelined then we won't block during the (implicit) read() below.
     * If the requests aren't pipelined, then the client is still waiting
     * for the final buffer flush from us, and we will block in the implicit
     * read().  B_SAFEREAD ensures that the BUFF layer flushes if it will
     * have to block during a read.
     */

    do {
        apr_status_t rv;

        /* ensure ap_rgetline allocates memory each time thru the loop
         * if there are empty lines
         */
        r->the_request = NULL;
        rv = ap_rgetline(&(r->the_request), (apr_size_t)(r->server->limit_req_line + 2),
                         &len, r, strict ? AP_GETLINE_CRLF : 0, bb);

        if (rv != APR_SUCCESS) {
            r->request_time = apr_time_now();

            /* ap_rgetline returns APR_ENOSPC if it fills up the
             * buffer before finding the end-of-line.  This is only going to
             * happen if it exceeds the configured limit for a request-line.
             */
            if (APR_STATUS_IS_ENOSPC(rv)) {
                r->status = HTTP_REQUEST_URI_TOO_LARGE;
            }
            else if (APR_STATUS_IS_TIMEUP(rv)) {
                r->status = HTTP_REQUEST_TIME_OUT;
            }
            else if (APR_STATUS_IS_EINVAL(rv)) {
                r->status = HTTP_BAD_REQUEST;
            }
            r->proto_num = HTTP_VERSION(1,0);
            r->protocol  = apr_pstrdup(r->pool, "HTTP/1.0");
            return 0;
        }
    } while ((len <= 0) && (--num_blank_lines >= 0));

    if (APLOGrtrace5(r)) {
        ap_log_rerror(APLOG_MARK, APLOG_TRACE5, 0, r,
                      "Request received from client: %s",
                      ap_escape_logitem(r->pool, r->the_request));
    }

    r->request_time = apr_time_now();

    r->method = r->the_request;

    /* If there is whitespace before a method, skip it and mark in error */
    if (apr_isspace(*r->method)) {
        deferred_error = rrl_badwhitespace; 
        for ( ; apr_isspace(*r->method); ++r->method)
            ; 
    }

    /* Scan the method up to the next whitespace, ensure it contains only
     * valid http-token characters, otherwise mark in error
     */
    if (strict) {
        ll = (char*) ap_scan_http_token(r->method);
    }
    else {
        ll = (char*) ap_scan_vchar_obstext(r->method);
    }

    if (((ll == r->method) || (*ll && !apr_isspace(*ll)))
            && deferred_error == rrl_none) {
        deferred_error = rrl_badmethod;
        ll = strpbrk(ll, "\t\n\v\f\r ");
    }

    /* Verify method terminated with a single SP, or mark as specific error */
    if (!ll) {
        if (deferred_error == rrl_none)
            deferred_error = rrl_missinguri;
        r->protocol = uri = "";
        len = 0;
        goto rrl_done;
    }
    else if (strict && ll[0] && apr_isspace(ll[1])
             && deferred_error == rrl_none) {
        deferred_error = rrl_excesswhitespace; 
    }

    /* Advance uri pointer over leading whitespace, NUL terminate the method
     * If non-SP whitespace is encountered, mark as specific error
     */
    for (uri = ll; apr_isspace(*uri); ++uri) 
        if (*uri != ' ' && deferred_error == rrl_none)
            deferred_error = rrl_badwhitespace; 
    *ll = '\0';

    if (!*uri && deferred_error == rrl_none)
        deferred_error = rrl_missinguri;

    /* Scan the URI up to the next whitespace, ensure it contains no raw
     * control characters, otherwise mark in error
     */
    ll = (char*) ap_scan_vchar_obstext(uri);
    if (ll == uri || (*ll && !apr_isspace(*ll))) {
        deferred_error = rrl_baduri;
        ll = strpbrk(ll, "\t\n\v\f\r ");
    }

    /* Verify URI terminated with a single SP, or mark as specific error */
    if (!ll) {
        r->protocol = "";
        len = 0;
        goto rrl_done;
    }
    else if (strict && ll[0] && apr_isspace(ll[1])
             && deferred_error == rrl_none) {
        deferred_error = rrl_excesswhitespace; 
    }

    /* Advance protocol pointer over leading whitespace, NUL terminate the uri
     * If non-SP whitespace is encountered, mark as specific error
     */
    for (r->protocol = ll; apr_isspace(*r->protocol); ++r->protocol) 
        if (*r->protocol != ' ' && deferred_error == rrl_none)
            deferred_error = rrl_badwhitespace; 
    *ll = '\0';

    /* Scan the protocol up to the next whitespace, validation comes later */
    if (!(ll = (char*) ap_scan_vchar_obstext(r->protocol))) {
        len = strlen(r->protocol);
        goto rrl_done;
    }
    len = ll - r->protocol;

    /* Advance over trailing whitespace, if found mark in error,
     * determine if trailing text is found, unconditionally mark in error,
     * finally NUL terminate the protocol string
     */
    if (*ll && !apr_isspace(*ll)) {
        deferred_error = rrl_badprotocol;
    }
    else if (strict && *ll) {
        deferred_error = rrl_excesswhitespace;
    }
    else {
        for ( ; apr_isspace(*ll); ++ll)
            if (*ll != ' ' && deferred_error == rrl_none)
                deferred_error = rrl_badwhitespace; 
        if (*ll && deferred_error == rrl_none)
            deferred_error = rrl_trailingtext;
    }
    *((char *)r->protocol + len) = '\0';

rrl_done:
    /* For internal integrity and palloc efficiency, reconstruct the_request
     * in one palloc, using only single SP characters, per spec.
     */
    r->the_request = apr_pstrcat(r->pool, r->method, *uri ? " " : NULL, uri,
                                 *r->protocol ? " " : NULL, r->protocol, NULL);

    if (len == 8
            && r->protocol[0] == 'H' && r->protocol[1] == 'T'
            && r->protocol[2] == 'T' && r->protocol[3] == 'P'
            && r->protocol[4] == '/' && apr_isdigit(r->protocol[5])
            && r->protocol[6] == '.' && apr_isdigit(r->protocol[7])
            && r->protocol[5] != '0') {
        r->assbackwards = 0;
        r->proto_num = HTTP_VERSION(r->protocol[5] - '0', r->protocol[7] - '0');
    }
    else if (len == 8
                 && (r->protocol[0] == 'H' || r->protocol[0] == 'h')
                 && (r->protocol[1] == 'T' || r->protocol[1] == 't')
                 && (r->protocol[2] == 'T' || r->protocol[2] == 't')
                 && (r->protocol[3] == 'P' || r->protocol[3] == 'p')
                 && r->protocol[4] == '/' && apr_isdigit(r->protocol[5])
                 && r->protocol[6] == '.' && apr_isdigit(r->protocol[7])
                 && r->protocol[5] != '0') {
        r->assbackwards = 0;
        r->proto_num = HTTP_VERSION(r->protocol[5] - '0', r->protocol[7] - '0');
        if (strict && deferred_error == rrl_none)
            deferred_error = rrl_badprotocol;
        else
            memcpy((char*)r->protocol, "HTTP", 4);
    }
    else if (r->protocol[0]) {
        r->proto_num = HTTP_VERSION(0, 9);
        /* Defer setting the r->protocol string till error msg is composed */
        if (deferred_error == rrl_none)
            deferred_error = rrl_badprotocol;
    }
    else {
        r->assbackwards = 1;
        r->protocol  = apr_pstrdup(r->pool, "HTTP/0.9");
        r->proto_num = HTTP_VERSION(0, 9);
    }

    /* Determine the method_number and parse the uri prior to invoking error
     * handling, such that these fields are available for substitution
     */
    r->method_number = ap_method_number_of(r->method);
    if (r->method_number == M_GET && r->method[0] == 'H')
        r->header_only = 1;

    ap_parse_uri(r, uri);

    /* With the request understood, we can consider HTTP/0.9 specific errors */
    if (r->proto_num == HTTP_VERSION(0, 9) && deferred_error == rrl_none) {
        if (conf->http09_enable == AP_HTTP09_DISABLE)
            deferred_error = rrl_reject09;
        else if (strict && (r->method_number != M_GET || r->header_only))
            deferred_error = rrl_badmethod09;
    }

    /* Now that the method, uri and protocol are all processed,
     * we can safely resume any deferred error reporting
     */
    if (deferred_error != rrl_none) {
        if (deferred_error == rrl_badmethod)
            ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, APLOGNO(03445)
                          "HTTP Request Line; Invalid method token: '%.*s'",
                          field_name_len(r->method), r->method);
        else if (deferred_error == rrl_badmethod09)
            ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, APLOGNO(03444)
                          "HTTP Request Line; Invalid method token: '%.*s'"
                          " (only GET is allowed for HTTP/0.9 requests)",
                          field_name_len(r->method), r->method);
        else if (deferred_error == rrl_missinguri)
            ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, APLOGNO(03446)
                          "HTTP Request Line; Missing URI");
        else if (deferred_error == rrl_baduri)
            ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, APLOGNO(03454)
                          "HTTP Request Line; URI incorrectly encoded: '%.*s'",
                          field_name_len(r->unparsed_uri), r->unparsed_uri);
        else if (deferred_error == rrl_badwhitespace)
            ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, APLOGNO(03447)
                          "HTTP Request Line; Invalid whitespace");
        else if (deferred_error == rrl_excesswhitespace)
            ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, APLOGNO(03448)
                          "HTTP Request Line; Excess whitespace "
                          "(disallowed by HttpProtocolOptions Strict)");
        else if (deferred_error == rrl_trailingtext)
            ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, APLOGNO(03449)
                          "HTTP Request Line; Extraneous text found '%.*s' "
                          "(perhaps whitespace was injected?)",
                          field_name_len(ll), ll);
        else if (deferred_error == rrl_reject09)
            ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, APLOGNO(02401)
                          "HTTP Request Line; Rejected HTTP/0.9 request");
        else if (deferred_error == rrl_badprotocol)
            ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, APLOGNO(02418)
                          "HTTP Request Line; Unrecognized protocol '%.*s' "
                          "(perhaps whitespace was injected?)",
                          field_name_len(r->protocol), r->protocol);
        r->status = HTTP_BAD_REQUEST;
        goto rrl_failed;
    }

    if (conf->http_methods == AP_HTTP_METHODS_REGISTERED
            && r->method_number == M_INVALID) {
        ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, APLOGNO(02423)
                      "HTTP Request Line; Unrecognized HTTP method: '%.*s' "
                      "(disallowed by RegisteredMethods)",
                      field_name_len(r->method), r->method);
        r->status = HTTP_NOT_IMPLEMENTED;
        /* This can't happen in an HTTP/0.9 request, we verified GET above */
        return 0;
    }

    if (r->status != HTTP_OK) {
        ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, APLOGNO(03450)
                      "HTTP Request Line; Unable to parse URI: '%.*s'",
                      field_name_len(r->uri), r->uri);
        goto rrl_failed;
    }

    if (strict) {
        if (r->parsed_uri.fragment) {
            /* RFC3986 3.5: no fragment */
            ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, APLOGNO(02421)
                          "HTTP Request Line; URI must not contain a fragment");
            r->status = HTTP_BAD_REQUEST;
            goto rrl_failed;
        }
        if (r->parsed_uri.user || r->parsed_uri.password) {
            ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, APLOGNO(02422)
                          "HTTP Request Line; URI must not contain a "
                          "username/password");
            r->status = HTTP_BAD_REQUEST;
            goto rrl_failed;
        }
    }

    return 1;

rrl_failed:
    if (r->proto_num == HTTP_VERSION(0, 9)) {
        /* Send all parsing and protocol error response with 1.x behavior,
         * and reserve 505 errors for actual HTTP protocols presented.
         * As called out in RFC7230 3.5, any errors parsing the protocol
         * from the request line are nearly always misencoded HTTP/1.x
         * requests. Only a valid 0.9 request with no parsing errors
         * at all may be treated as a simple request, if allowed.
         */
        r->assbackwards = 0;
        r->connection->keepalive = AP_CONN_CLOSE;
        r->proto_num = HTTP_VERSION(1, 0);
        r->protocol  = apr_pstrdup(r->pool, "HTTP/1.0");
    }
    return 0;
}