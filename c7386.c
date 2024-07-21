request_rec *h2_request_create_rec(const h2_request *req, conn_rec *c)
{
    int access_status = HTTP_OK;    
    const char *rpath;
    const char *s;

#if AP_MODULE_MAGIC_AT_LEAST(20150222, 13)
    request_rec *r = ap_create_request(c);
#else
    request_rec *r = my_ap_create_request(c);
#endif

    r->headers_in = apr_table_clone(r->pool, req->headers);

    ap_run_pre_read_request(r, c);
    
    /* Time to populate r with the data we have. */
    r->request_time = req->request_time;
    r->method = req->method;
    /* Provide quick information about the request method as soon as known */
    r->method_number = ap_method_number_of(r->method);
    if (r->method_number == M_GET && r->method[0] == 'H') {
        r->header_only = 1;
    }

    rpath = (req->path ? req->path : "");
    ap_parse_uri(r, rpath);
    r->protocol = (char*)"HTTP/2.0";
    r->proto_num = HTTP_VERSION(2, 0);

    r->the_request = apr_psprintf(r->pool, "%s %s %s", 
                                  r->method, rpath, r->protocol);
    
    /* update what we think the virtual host is based on the headers we've
     * now read. may update status.
     * Leave r->hostname empty, vhost will parse if form our Host: header,
     * otherwise we get complains about port numbers.
     */
    r->hostname = NULL;
    ap_update_vhost_from_headers(r);
    
    /* we may have switched to another server */
    r->per_dir_config = r->server->lookup_defaults;
    
    s = apr_table_get(r->headers_in, "Expect");
    if (s && s[0]) {
        if (ap_cstr_casecmp(s, "100-continue") == 0) {
            r->expecting_100 = 1;
        }
        else {
            r->status = HTTP_EXPECTATION_FAILED;
            ap_send_error_response(r, 0);
        }
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
        /* Request check post hooks failed. An example of this would be a
         * request for a vhost where h2 is disabled --> 421.
         */
        ap_log_cerror(APLOG_MARK, APLOG_DEBUG, 0, c, APLOGNO(03367)
                      "h2_request: access_status=%d, request_create failed",
                      access_status);
        ap_die(access_status, r);
        ap_update_child_status(c->sbh, SERVER_BUSY_LOG, r);
        ap_run_log_transaction(r);
        r = NULL;
        goto traceout;
    }

    AP_READ_REQUEST_SUCCESS((uintptr_t)r, (char *)r->method, 
                            (char *)r->uri, (char *)r->server->defn_name, 
                            r->status);
    return r;
traceout:
    AP_READ_REQUEST_FAILURE((uintptr_t)r);
    return r;
}