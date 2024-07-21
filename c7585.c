static int am_handle_login(request_rec *r)
{
    am_dir_cfg_rec *cfg = am_get_dir_cfg(r);
    char *idp_param;
    const char *idp;
    char *return_to;
    int is_passive;
    int ret;

    return_to = am_extract_query_parameter(r->pool, r->args, "ReturnTo");
    if(return_to == NULL) {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
                      "Missing required ReturnTo parameter.");
        return HTTP_BAD_REQUEST;
    }

    ret = am_urldecode(return_to);
    if(ret != OK) {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
                      "Error urldecoding ReturnTo parameter.");
        return ret;
    }

    idp_param = am_extract_query_parameter(r->pool, r->args, "IdP");
    if(idp_param != NULL) {
        ret = am_urldecode(idp_param);
        if(ret != OK) {
            ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
                          "Error urldecoding IdP parameter.");
            return ret;
        }
    }

    ret = am_get_boolean_query_parameter(r, "IsPassive", &is_passive, FALSE);
    if (ret != OK) {
        return ret;
    }

    if(idp_param != NULL) {
        idp = idp_param;
    } else if(cfg->discovery_url) {
        if(is_passive) {
            /* We cannot currently do discovery with passive authentication requests. */
            ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
                          "Discovery service with passive authentication request unsupported.");
            return HTTP_INTERNAL_SERVER_ERROR;
        }
        return am_start_disco(r, return_to);
    } else {
        /* No discovery service -- just use the default IdP. */
        idp = am_get_idp(r);
    }

    return am_send_login_authn_request(r, idp, return_to, is_passive);
}