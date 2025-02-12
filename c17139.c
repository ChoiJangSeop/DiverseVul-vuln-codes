request_rec *h2_request_create_rec(const h2_request *req, conn_rec *c)
{
    int access_status;

#if AP_MODULE_MAGIC_AT_LEAST(20150222, 13)
    request_rec *r = ap_create_request(c);
#else
    request_rec *r = my_ap_create_request(c);
#endif

#if AP_MODULE_MAGIC_AT_LEAST(20200331, 3)
    ap_run_pre_read_request(r, c);

    /* Time to populate r with the data we have. */
    r->request_time = req->request_time;
    r->the_request = apr_psprintf(r->pool, "%s %s HTTP/2.0",
                                  req->method, req->path ? req->path : "");
    r->headers_in = apr_table_clone(r->pool, req->headers);

    /* Start with r->hostname = NULL, ap_check_request_header() will get it
     * form Host: header, otherwise we get complains about port numbers.
     */
    r->hostname = NULL;

    /* Validate HTTP/1 request and select vhost. */
    if (!ap_parse_request_line(r) || !ap_check_request_header(r)) {
        /* we may have switched to another server still */
        r->per_dir_config = r->server->lookup_defaults;
        if (req->http_status != H2_HTTP_STATUS_UNSET) {
            access_status = req->http_status;
            /* Be safe and close the connection */
            c->keepalive = AP_CONN_CLOSE;
        }
        else {
            access_status = r->status;
        }
        r->status = HTTP_OK;
        goto die;
    }
#else
    {
        const char *s;

        r->headers_in = apr_table_clone(r->pool, req->headers);
        ap_run_pre_read_request(r, c);

        /* Time to populate r with the data we have. */
        r->request_time = req->request_time;
        r->method = apr_pstrdup(r->pool, req->method);
        /* Provide quick information about the request method as soon as known */
        r->method_number = ap_method_number_of(r->method);
        if (r->method_number == M_GET && r->method[0] == 'H') {
            r->header_only = 1;
        }
        ap_parse_uri(r, req->path ? req->path : "");
        r->protocol = (char*)"HTTP/2.0";
        r->proto_num = HTTP_VERSION(2, 0);
        r->the_request = apr_psprintf(r->pool, "%s %s HTTP/2.0",
                                      r->method, req->path ? req->path : "");

        /* Start with r->hostname = NULL, ap_check_request_header() will get it
         * form Host: header, otherwise we get complains about port numbers.
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
                access_status = r->status;
                goto die;
            }
         }
    }
#endif

    /* we may have switched to another server */
    r->per_dir_config = r->server->lookup_defaults;

    if (req->http_status != H2_HTTP_STATUS_UNSET) {
        access_status = req->http_status;
        r->status = HTTP_OK;
        /* Be safe and close the connection */
        c->keepalive = AP_CONN_CLOSE;
        goto die;
    }

    /*
     * Add the HTTP_IN filter here to ensure that ap_discard_request_body
     * called by ap_die and by ap_send_error_response works correctly on
     * status codes that do not cause the connection to be dropped and
     * in situations where the connection should be kept alive.
     */
    ap_add_input_filter_handle(ap_http_input_filter_handle,
                               NULL, r, r->connection);
    
    if ((access_status = ap_run_post_read_request(r))) {
        /* Request check post hooks failed. An example of this would be a
         * request for a vhost where h2 is disabled --> 421.
         */
        ap_log_cerror(APLOG_MARK, APLOG_DEBUG, 0, c, APLOGNO(03367)
                      "h2_request: access_status=%d, request_create failed",
                      access_status);
        goto die;
    }

    AP_READ_REQUEST_SUCCESS((uintptr_t)r, (char *)r->method, 
                            (char *)r->uri, (char *)r->server->defn_name, 
                            r->status);
    return r;

die:
    ap_die(access_status, r);

    /* ap_die() sent the response through the output filters, we must now
     * end the request with an EOR bucket for stream/pipeline accounting.
     */
    {
        apr_bucket_brigade *eor_bb;
#if AP_MODULE_MAGIC_AT_LEAST(20180905, 1)
        eor_bb = ap_acquire_brigade(c);
        APR_BRIGADE_INSERT_TAIL(eor_bb,
                                ap_bucket_eor_create(c->bucket_alloc, r));
        ap_pass_brigade(c->output_filters, eor_bb);
        ap_release_brigade(c, eor_bb);
#else
        eor_bb = apr_brigade_create(c->pool, c->bucket_alloc);
        APR_BRIGADE_INSERT_TAIL(eor_bb,
                                ap_bucket_eor_create(c->bucket_alloc, r));
        ap_pass_brigade(c->output_filters, eor_bb);
        apr_brigade_destroy(eor_bb);
#endif
    }

    r = NULL;
    AP_READ_REQUEST_FAILURE((uintptr_t)r);
    return NULL;
}