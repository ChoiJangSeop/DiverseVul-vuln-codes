static int am_handle_probe_discovery(request_rec *r) {
    am_dir_cfg_rec *cfg = am_get_dir_cfg(r);
    LassoServer *server;
    const char *disco_idp = NULL;
    int timeout;
    char *return_to;
    char *idp_param;
    char *redirect_url;
    int ret;

    server = am_get_lasso_server(r);
    if(server == NULL) {
        return HTTP_INTERNAL_SERVER_ERROR;
    }

    /*
     * If built-in IdP discovery is not configured, return error.
     * For now we only have the get-metadata metadata method, so this
     * information is not saved in configuration nor it is checked here.
     */
    timeout = cfg->probe_discovery_timeout;
    if (timeout == -1) {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
                      "probe discovery handler invoked but not "
                      "configured. Plase set MellonProbeDiscoveryTimeout.");
        return HTTP_INTERNAL_SERVER_ERROR;
    }

    /*
     * Check for mandatory arguments early to avoid sending 
     * probles for nothing.
     */
    return_to = am_extract_query_parameter(r->pool, r->args, "return");
    if(return_to == NULL) {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
                      "Missing required return parameter.");
        return HTTP_BAD_REQUEST;
    }

    ret = am_urldecode(return_to);
    if (ret != OK) {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, ret, r,
                      "Could not urldecode return value.");
        return HTTP_BAD_REQUEST;
    }

    idp_param = am_extract_query_parameter(r->pool, r->args, "returnIDParam");
    if(idp_param == NULL) {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
                      "Missing required returnIDParam parameter.");
        return HTTP_BAD_REQUEST;
    }

    ret = am_urldecode(idp_param);
    if (ret != OK) {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, ret, r,
                      "Could not urldecode returnIDParam value.");
        return HTTP_BAD_REQUEST;
    }

    /*
     * Proceed with built-in IdP discovery. 
     *
     * First try sending probes to IdP configured for discovery.
     * Second send probes for all configured IdP
     * The first to answer is chosen.
     * If none answer, use the first configured IdP
     */
    if (!apr_is_empty_table(cfg->probe_discovery_idp)) {
        const apr_array_header_t *header;
        apr_table_entry_t *elts;
        const char *url;
        const char *idp;
        int i;

        header = apr_table_elts(cfg->probe_discovery_idp);
        elts = (apr_table_entry_t *)header->elts;

        for (i = 0; i < header->nelts; i++) { 
            idp = elts[i].key;
            url = elts[i].val;

            if (am_probe_url(r, url, timeout) == OK) {
                disco_idp = idp;
                break;
            }
        }
    } else {
        GList *iter;
        GList *idp_list;
        const char *idp;

        idp_list = g_hash_table_get_keys(server->providers);
        for (iter = idp_list; iter != NULL; iter = iter->next) {
            idp = iter->data;
    
            if (am_probe_url(r, idp, timeout) == OK) {
                disco_idp = idp;
                break;
            }
        }
        g_list_free(idp_list);
    }

    /* 
     * On failure, try default
     */
    if (disco_idp == NULL) {
        disco_idp = am_first_idp(r);
        if (disco_idp == NULL) {
            ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, 
                          "probeDiscovery found no usable IdP.");
            return HTTP_INTERNAL_SERVER_ERROR;
        } else {
            ap_log_rerror(APLOG_MARK, APLOG_WARNING, 0, r, "probeDiscovery "
                          "failed, trying default IdP %s", disco_idp); 
        }
    } else {
        ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r,
                      "probeDiscovery using %s", disco_idp);
    }

    redirect_url = apr_psprintf(r->pool, "%s%s%s=%s", return_to, 
                                strchr(return_to, '?') ? "&" : "?",
                                am_urlencode(r->pool, idp_param), 
                                am_urlencode(r->pool, disco_idp));

    apr_table_setn(r->headers_out, "Location", redirect_url);

    return HTTP_SEE_OTHER;
}