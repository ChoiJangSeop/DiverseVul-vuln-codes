static int am_handle_logout_request(request_rec *r, 
                                    LassoLogout *logout, char *msg)
{
    gint res = 0, rc = HTTP_OK;
    am_cache_entry_t *session = NULL;
    am_dir_cfg_rec *cfg = am_get_dir_cfg(r);

    /* Process the logout message. Ignore missing signature. */
    res = lasso_logout_process_request_msg(logout, msg);
#ifdef HAVE_lasso_profile_set_signature_verify_hint
    if(res != 0 && res != LASSO_DS_ERROR_SIGNATURE_NOT_FOUND) {
        if (apr_hash_get(cfg->do_not_verify_logout_signature,
                         logout->parent.remote_providerID,
                         APR_HASH_KEY_STRING)) {
            lasso_profile_set_signature_verify_hint(&logout->parent,
                LASSO_PROFILE_SIGNATURE_VERIFY_HINT_IGNORE);
            res = lasso_logout_process_request_msg(logout, msg);
        }
    }
#endif
    if(res != 0 && res != LASSO_DS_ERROR_SIGNATURE_NOT_FOUND) {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
                      "Error processing logout request message."
                      " Lasso error: [%i] %s", res, lasso_strerror(res));

        rc = HTTP_BAD_REQUEST;
        goto exit;
    }

    /* Search session using NameID */
    if (! LASSO_IS_SAML2_NAME_ID(logout->parent.nameIdentifier)) {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
                      "Error processing logout request message."
                      " No NameID found");
        rc = HTTP_BAD_REQUEST;
        goto exit;
    }
    session = am_get_request_session_by_nameid(r,
                    ((LassoSaml2NameID*)logout->parent.nameIdentifier)->content);
    if (session == NULL) {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
                      "Error processing logout request message."
                      " No session found for NameID %s",
                      ((LassoSaml2NameID*)logout->parent.nameIdentifier)->content);

    }
    if (session == NULL) {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
                      "Error processing logout request message."
                      " No session found.");

    } else {
        am_restore_lasso_profile_state(r, &logout->parent, session);
    }

    /* Validate the logout message. Ignore missing signature. */
    res = lasso_logout_validate_request(logout);
    if(res != 0 && 
       res != LASSO_DS_ERROR_SIGNATURE_NOT_FOUND &&
       res != LASSO_PROFILE_ERROR_SESSION_NOT_FOUND) {
        ap_log_rerror(APLOG_MARK, APLOG_WARNING, 0, r,
                      "Error validating logout request."
                      " Lasso error: [%i] %s", res, lasso_strerror(res));
        rc = HTTP_INTERNAL_SERVER_ERROR;
        goto exit;
    }
    /* We continue with the logout despite those errors. They could be
     * caused by the IdP believing that we are logged in when we are not.
     */

    if (session != NULL && res != LASSO_PROFILE_ERROR_SESSION_NOT_FOUND) {
        /* We found a matching session -- delete it. */
        am_delete_request_session(r, session);
        session = NULL;
    }

    /* Create response message. */
    res = lasso_logout_build_response_msg(logout);
    if(res != 0) {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
                      "Error building logout response message."
                      " Lasso error: [%i] %s", res, lasso_strerror(res));

        rc = HTTP_INTERNAL_SERVER_ERROR;
        goto exit;
    }
    rc = am_return_logout_response(r, &logout->parent);

exit:
    if (session != NULL) {
        am_release_request_session(r, session);
    }

    lasso_logout_destroy(logout);
    return rc;
}