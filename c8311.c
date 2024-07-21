static int auth_check_hdrs(struct transaction_t *txn, int *sasl_result)
{
    int ret = 0, r = 0;
    const char **hdr;

    if (txn->flags.redirect) return 0;

    /* Perform authentication, if necessary */
    if ((hdr = spool_getheader(txn->req_hdrs, "Authorization"))) {
        if (httpd_userid) {
            /* Reauth - reinitialize */
            syslog(LOG_DEBUG, "reauth - reinit");
            reset_saslconn(&httpd_saslconn);
            txn->auth_chal.scheme = NULL;
        }

        if (httpd_tls_required) {
            /* TLS required - redirect handled below */
            ret = HTTP_UNAUTHORIZED;
        }
        else {
            /* Check the auth credentials */
            r = http_auth(hdr[0], txn);
            if ((r < 0) || !txn->auth_chal.scheme) {
                /* Auth failed - reinitialize */
                syslog(LOG_DEBUG, "auth failed - reinit");
                reset_saslconn(&httpd_saslconn);
                txn->auth_chal.scheme = NULL;
                if (r == SASL_UNAVAIL) {
                    /* The namespace to authenticate to is unavailable.
                     * There could be any reason for this, e.g. the DAV
                     * handler could have run into a timeout for the
                     * user's dabatase. In any case, there's no sense
                     * to challenge the client for authentication. */
                    ret = HTTP_UNAVAILABLE;
                }
                else if (r == SASL_FAIL) {
                    ret = HTTP_SERVER_ERROR;
                }
                else {
                    ret = HTTP_UNAUTHORIZED;
                }
            }
            else if (r == SASL_CONTINUE) {
                /* Continue with multi-step authentication */
                ret = HTTP_UNAUTHORIZED;
            }
        }
    }
    else if (!httpd_userid && txn->auth_chal.scheme) {
        /* Started auth exchange, but client didn't engage - reinit */
        syslog(LOG_DEBUG, "client didn't complete auth - reinit");
        reset_saslconn(&httpd_saslconn);
        txn->auth_chal.scheme = NULL;
    }

    /* Perform proxy authorization, if necessary */
    else if (httpd_authid &&
             (hdr = spool_getheader(txn->req_hdrs, "Authorize-As")) &&
             *hdr[0]) {
        const char *authzid = hdr[0];

        r = proxy_authz(&authzid, txn);
        if (r) {
            /* Proxy authz failed - reinitialize */
            syslog(LOG_DEBUG, "proxy authz failed - reinit");
            reset_saslconn(&httpd_saslconn);
            txn->auth_chal.scheme = NULL;
            ret = HTTP_UNAUTHORIZED;
        }
        else {
            ret = auth_success(txn, authzid);
        }
    }

    *sasl_result = r;

    return ret;
}