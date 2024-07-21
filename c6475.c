static int md_http_challenge_pr(request_rec *r)
{
    apr_bucket_brigade *bb;
    const md_srv_conf_t *sc;
    const char *name, *data;
    md_reg_t *reg;
    int configured;
    apr_status_t rv;
    
    if (!strncmp(ACME_CHALLENGE_PREFIX, r->parsed_uri.path, sizeof(ACME_CHALLENGE_PREFIX)-1)) {
        sc = ap_get_module_config(r->server->module_config, &md_module);
        if (sc && sc->mc) {
            configured = (NULL != md_get_by_domain(sc->mc->mds, r->hostname));
            if (r->method_number == M_GET) {
                name = r->parsed_uri.path + sizeof(ACME_CHALLENGE_PREFIX)-1;
                reg = sc && sc->mc? sc->mc->reg : NULL;
                
                r->status = HTTP_NOT_FOUND;
                if (!ap_strchr_c(name, '/') && reg) {
                    md_store_t *store = md_reg_store_get(reg);
                    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, 
                                  "Challenge for %s (%s)", r->hostname, r->uri);
                    
                    rv = md_store_load(store, MD_SG_CHALLENGES, r->hostname, 
                                       MD_FN_HTTP01, MD_SV_TEXT, (void**)&data, r->pool);
                    if (APR_SUCCESS == rv) {
                        apr_size_t len = strlen(data);
                        
                        r->status = HTTP_OK;
                        apr_table_setn(r->headers_out, "Content-Length", apr_ltoa(r->pool, (long)len));
                        
                        bb = apr_brigade_create(r->pool, r->connection->bucket_alloc);
                        apr_brigade_write(bb, NULL, NULL, data, len);
                        ap_pass_brigade(r->output_filters, bb);
                        apr_brigade_cleanup(bb);
                    }
                    else if (!configured) {
                        /* The request hostname is not for a configured domain. We are not
                         * the sole authority here for /.well-known/acme-challenge (see PR62189).
                         * So, we decline to handle this and let others step in.
                         */
                        return DECLINED;
                    }
                    else if (APR_STATUS_IS_ENOENT(rv)) {
                        return HTTP_NOT_FOUND;
                    }
                    else if (APR_ENOENT != rv) {
                        ap_log_rerror(APLOG_MARK, APLOG_ERR, rv, r, APLOGNO(10081)
                                      "loading challenge %s from store", name);
                        return HTTP_INTERNAL_SERVER_ERROR;
                    }
                }
                return r->status;
            }
            else if (configured) {
                /* See comment above, we prevent any other access only for domains
                 * the have been configured for mod_md. */ 
                return HTTP_NOT_IMPLEMENTED;
            }
        }
        
    }
    return DECLINED;
}