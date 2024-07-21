request_rec *ap_read_request(conn_rec *conn)
{
    request_rec *r;
    apr_pool_t *p;
    const char *expect;
    int access_status;
    apr_bucket_brigade *tmp_bb;
    apr_socket_t *csd;
    apr_interval_time_t cur_timeout;


    apr_pool_create(&p, conn->pool);
    apr_pool_tag(p, "request");
    r = apr_pcalloc(p, sizeof(request_rec));
    AP_READ_REQUEST_ENTRY((intptr_t)r, (uintptr_t)conn);
    r->pool            = p;
    r->connection      = conn;
    r->server          = conn->base_server;

    r->user            = NULL;
    r->ap_auth_type    = NULL;

    r->allowed_methods = ap_make_method_list(p, 2);

    r->headers_in      = apr_table_make(r->pool, 25);
    r->trailers_in     = apr_table_make(r->pool, 5);
    r->subprocess_env  = apr_table_make(r->pool, 25);
    r->headers_out     = apr_table_make(r->pool, 12);
    r->err_headers_out = apr_table_make(r->pool, 5);
    r->trailers_out    = apr_table_make(r->pool, 5);
    r->notes           = apr_table_make(r->pool, 5);

    r->request_config  = ap_create_request_config(r->pool);
    /* Must be set before we run create request hook */

    r->proto_output_filters = conn->output_filters;
    r->output_filters  = r->proto_output_filters;
    r->proto_input_filters = conn->input_filters;
    r->input_filters   = r->proto_input_filters;
    ap_run_create_request(r);
    r->per_dir_config  = r->server->lookup_defaults;

    r->sent_bodyct     = 0;                      /* bytect isn't for body */

    r->read_length     = 0;
    r->read_body       = REQUEST_NO_BODY;

    r->status          = HTTP_OK;  /* Until further notice */
    r->the_request     = NULL;

    /* Begin by presuming any module can make its own path_info assumptions,
     * until some module interjects and changes the value.
     */
    r->used_path_info = AP_REQ_DEFAULT_PATH_INFO;

    r->useragent_addr = conn->client_addr;
    r->useragent_ip = conn->client_ip;

    tmp_bb = apr_brigade_create(r->pool, r->connection->bucket_alloc);
    conn->keepalive = AP_CONN_UNKNOWN;

    ap_run_pre_read_request(r, conn);

    /* Get the request... */
    if (!read_request_line(r, tmp_bb)) {
        switch (r->status) {
        case HTTP_REQUEST_URI_TOO_LARGE:
        case HTTP_BAD_REQUEST:
        case HTTP_VERSION_NOT_SUPPORTED:
        case HTTP_NOT_IMPLEMENTED:
            if (r->status == HTTP_REQUEST_URI_TOO_LARGE) {
                ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, APLOGNO(00565)
                              "request failed: client's request-line exceeds LimitRequestLine (longer than %d)",
                              r->server->limit_req_line);
            }
            else if (r->method == NULL) {
                ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, APLOGNO(00566)
                              "request failed: malformed request line");
            }
            access_status = r->status;
            r->status = HTTP_OK;
            ap_die(access_status, r);
            ap_update_child_status(conn->sbh, SERVER_BUSY_LOG, r);
            ap_run_log_transaction(r);
            r = NULL;
            apr_brigade_destroy(tmp_bb);
            goto traceout;
        case HTTP_REQUEST_TIME_OUT:
            ap_update_child_status(conn->sbh, SERVER_BUSY_LOG, NULL);
            if (!r->connection->keepalives)
                ap_run_log_transaction(r);
            apr_brigade_destroy(tmp_bb);
            goto traceout;
        default:
            apr_brigade_destroy(tmp_bb);
            r = NULL;
            goto traceout;
        }
    }

    /* We may have been in keep_alive_timeout mode, so toggle back
     * to the normal timeout mode as we fetch the header lines,
     * as necessary.
     */
    csd = ap_get_conn_socket(conn);
    apr_socket_timeout_get(csd, &cur_timeout);
    if (cur_timeout != conn->base_server->timeout) {
        apr_socket_timeout_set(csd, conn->base_server->timeout);
        cur_timeout = conn->base_server->timeout;
    }

    if (!r->assbackwards) {
        const char *tenc, *clen;

        ap_get_mime_headers_core(r, tmp_bb);
        if (r->status != HTTP_OK) {
            ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, APLOGNO(00567)
                          "request failed: error reading the headers");
            ap_send_error_response(r, 0);
            ap_update_child_status(conn->sbh, SERVER_BUSY_LOG, r);
            ap_run_log_transaction(r);
            apr_brigade_destroy(tmp_bb);
            goto traceout;
        }

        clen = apr_table_get(r->headers_in, "Content-Length");
        if (clen) {
            apr_off_t cl;

            if (!ap_parse_strict_length(&cl, clen)) {
                ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, APLOGNO(10242)
                              "client sent invalid Content-Length "
                              "(%s): %s", clen, r->uri);
                r->status = HTTP_BAD_REQUEST;
                conn->keepalive = AP_CONN_CLOSE;
                ap_send_error_response(r, 0);
                ap_update_child_status(conn->sbh, SERVER_BUSY_LOG, r);
                ap_run_log_transaction(r);
                apr_brigade_destroy(tmp_bb);
                goto traceout;
            }
        }

        tenc = apr_table_get(r->headers_in, "Transfer-Encoding");
        if (tenc) {
            /* https://tools.ietf.org/html/rfc7230
             * Section 3.3.3.3: "If a Transfer-Encoding header field is
             * present in a request and the chunked transfer coding is not
             * the final encoding ...; the server MUST respond with the 400
             * (Bad Request) status code and then close the connection".
             */
            if (!ap_is_chunked(r->pool, tenc)) {
                ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, APLOGNO(02539)
                              "client sent unknown Transfer-Encoding "
                              "(%s): %s", tenc, r->uri);
                r->status = HTTP_BAD_REQUEST;
                conn->keepalive = AP_CONN_CLOSE;
                ap_send_error_response(r, 0);
                ap_update_child_status(conn->sbh, SERVER_BUSY_LOG, r);
                ap_run_log_transaction(r);
                apr_brigade_destroy(tmp_bb);
                goto traceout;
            }

            /* https://tools.ietf.org/html/rfc7230
             * Section 3.3.3.3: "If a message is received with both a
             * Transfer-Encoding and a Content-Length header field, the
             * Transfer-Encoding overrides the Content-Length. ... A sender
             * MUST remove the received Content-Length field".
             */
            if (clen) {
                apr_table_unset(r->headers_in, "Content-Length");

                /* Don't reuse this connection anyway to avoid confusion with
                 * intermediaries and request/reponse spltting.
                 */
                conn->keepalive = AP_CONN_CLOSE;
            }
        }
    }

    apr_brigade_destroy(tmp_bb);

    /* update what we think the virtual host is based on the headers we've
     * now read. may update status.
     */
    ap_update_vhost_from_headers(r);
    access_status = r->status;

    /* Toggle to the Host:-based vhost's timeout mode to fetch the
     * request body and send the response body, if needed.
     */
    if (cur_timeout != r->server->timeout) {
        apr_socket_timeout_set(csd, r->server->timeout);
        cur_timeout = r->server->timeout;
    }

    /* we may have switched to another server */
    r->per_dir_config = r->server->lookup_defaults;

    if ((!r->hostname && (r->proto_num >= HTTP_VERSION(1, 1)))
        || ((r->proto_num == HTTP_VERSION(1, 1))
            && !apr_table_get(r->headers_in, "Host"))) {
        /*
         * Client sent us an HTTP/1.1 or later request without telling us the
         * hostname, either with a full URL or a Host: header. We therefore
         * need to (as per the 1.1 spec) send an error.  As a special case,
         * HTTP/1.1 mentions twice (S9, S14.23) that a request MUST contain
         * a Host: header, and the server MUST respond with 400 if it doesn't.
         */
        access_status = HTTP_BAD_REQUEST;
        ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, APLOGNO(00569)
                      "client sent HTTP/1.1 request without hostname "
                      "(see RFC2616 section 14.23): %s", r->uri);
    }

    /*
     * Add the HTTP_IN filter here to ensure that ap_discard_request_body
     * called by ap_die and by ap_send_error_response works correctly on
     * status codes that do not cause the connection to be dropped and
     * in situations where the connection should be kept alive.
     */

    ap_add_input_filter_handle(ap_http_input_filter_handle,
                               NULL, r, r->connection);

    if (access_status != HTTP_OK
        || (access_status = ap_run_post_read_request(r))) {
        ap_die(access_status, r);
        ap_update_child_status(conn->sbh, SERVER_BUSY_LOG, r);
        ap_run_log_transaction(r);
        r = NULL;
        goto traceout;
    }

    if (((expect = apr_table_get(r->headers_in, "Expect")) != NULL)
        && (expect[0] != '\0')) {
        /*
         * The Expect header field was added to HTTP/1.1 after RFC 2068
         * as a means to signal when a 100 response is desired and,
         * unfortunately, to signal a poor man's mandatory extension that
         * the server must understand or return 417 Expectation Failed.
         */
        if (ap_cstr_casecmp(expect, "100-continue") == 0) {
            r->expecting_100 = 1;
        }
        else {
            r->status = HTTP_EXPECTATION_FAILED;
            ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, APLOGNO(00570)
                          "client sent an unrecognized expectation value of "
                          "Expect: %s", expect);
            ap_send_error_response(r, 0);
            ap_update_child_status(conn->sbh, SERVER_BUSY_LOG, r);
            ap_run_log_transaction(r);
            goto traceout;
        }
    }

    AP_READ_REQUEST_SUCCESS((uintptr_t)r, (char *)r->method, (char *)r->uri, (char *)r->server->defn_name, r->status);
    return r;
    traceout:
    AP_READ_REQUEST_FAILURE((uintptr_t)r);
    return r;
}