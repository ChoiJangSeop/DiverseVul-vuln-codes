static int am_handle_logout_response(request_rec *r, LassoLogout *logout)
{
    gint res;
    int rc;
    am_cache_entry_t *session;
    char *return_to;
    am_dir_cfg_rec *cfg = am_get_dir_cfg(r);

    res = lasso_logout_process_response_msg(logout, r->args);
#ifdef HAVE_lasso_profile_set_signature_verify_hint
    if(res != 0 && res != LASSO_DS_ERROR_SIGNATURE_NOT_FOUND) {
        if (apr_hash_get(cfg->do_not_verify_logout_signature,
                         logout->parent.remote_providerID,
                         APR_HASH_KEY_STRING)) {
            lasso_profile_set_signature_verify_hint(&logout->parent,
                LASSO_PROFILE_SIGNATURE_VERIFY_HINT_IGNORE);
            res = lasso_logout_process_response_msg(logout, r->args);
        }
    }
#endif
    if(res != 0) {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
                      "Unable to process logout response."
                      " Lasso error: [%i] %s", res, lasso_strerror(res));

        lasso_logout_destroy(logout);
        return HTTP_BAD_REQUEST;
    }

    lasso_logout_destroy(logout);

    /* Delete the session. */
    session = am_get_request_session(r);
    if(session != NULL) {
        am_delete_request_session(r, session);
    }

    return_to = am_extract_query_parameter(r->pool, r->args, "RelayState");
    if(return_to == NULL) {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
                      "No RelayState parameter to logout response handler."
                      " It is possible that your IdP doesn't support the"
                      " RelayState parameter.");
        return HTTP_BAD_REQUEST;
    }

    rc = am_urldecode(return_to);
    if(rc != OK) {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, rc, r,
                      "Could not urldecode RelayState value in logout"
                      " response.");
        return HTTP_BAD_REQUEST;
    }

    /* Check for bad characters in RelayState. */
    rc = am_check_url(r, return_to);
    if (rc != OK) {
        return rc;
    }

    apr_table_setn(r->headers_out, "Location", return_to);
    return HTTP_SEE_OTHER;
}