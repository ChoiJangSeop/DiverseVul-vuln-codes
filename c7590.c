static int am_init_logout_request(request_rec *r, LassoLogout *logout)
{
    char *return_to;
    int rc;
    am_cache_entry_t *mellon_session;
    gint res;
    char *redirect_to;
    LassoProfile *profile;
    LassoSession *session;
    GList *assertion_list;
    LassoNode *assertion_n;
    LassoSaml2Assertion *assertion;
    LassoSaml2AuthnStatement *authnStatement;
    LassoSamlp2LogoutRequest *request;

    return_to = am_extract_query_parameter(r->pool, r->args, "ReturnTo");
    rc = am_urldecode(return_to);
    if (rc != OK) {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, rc, r,
                      "Could not urldecode ReturnTo value.");
        return HTTP_BAD_REQUEST;
    }

    /* Disable the the local session (in case the IdP doesn't respond). */
    mellon_session = am_get_request_session(r);
    if(mellon_session != NULL) {
        am_restore_lasso_profile_state(r, &logout->parent, mellon_session);
        mellon_session->logged_in = 0;
        am_release_request_session(r, mellon_session);
    }

    /* Create the logout request message. */
    res = lasso_logout_init_request(logout, NULL, LASSO_HTTP_METHOD_REDIRECT);
    /* Early non failing return. */
    if (res != 0) {
        if(res == LASSO_PROFILE_ERROR_SESSION_NOT_FOUND) {
            ap_log_rerror(APLOG_MARK, APLOG_WARNING, 0, r,
                          "User attempted to initiate logout without being"
                          " loggged in.");
        } else if (res == LASSO_LOGOUT_ERROR_UNSUPPORTED_PROFILE || res == LASSO_PROFILE_ERROR_UNSUPPORTED_PROFILE) {
            ap_log_rerror(APLOG_MARK, APLOG_WARNING, 0, r, "Current identity provider "
                            "does not support single logout. Destroying local session only.");

        } else if(res != 0) {
            ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
                          "Unable to create logout request."
                          " Lasso error: [%i] %s", res, lasso_strerror(res));

            lasso_logout_destroy(logout);
            return HTTP_INTERNAL_SERVER_ERROR;
        }
        lasso_logout_destroy(logout);
        /* Check for bad characters in ReturnTo. */
        rc = am_check_url(r, return_to);
        if (rc != OK) {
            return rc;
        }
        /* Redirect to the page the user should be sent to after logout. */
        apr_table_setn(r->headers_out, "Location", return_to);
        return HTTP_SEE_OTHER;
    }

    profile = LASSO_PROFILE(logout);

    /* We need to set the SessionIndex in the LogoutRequest to the SessionIndex
     * we received during the login operation. This is not needed since release
     * 2.3.0.
     */
    if (lasso_check_version(2, 3, 0, LASSO_CHECK_VERSION_NUMERIC) == 0) {
        session = lasso_profile_get_session(profile);
        assertion_list = lasso_session_get_assertions(
            session, profile->remote_providerID);
        if(! assertion_list ||
                        LASSO_IS_SAML2_ASSERTION(assertion_list->data) == FALSE) {
            ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
                          "No assertions found for the current session.");
            lasso_logout_destroy(logout);
            return HTTP_INTERNAL_SERVER_ERROR;
        }
        /* We currently only look at the first assertion in the list
         * lasso_session_get_assertions returns.
         */
        assertion_n = assertion_list->data;

        assertion = LASSO_SAML2_ASSERTION(assertion_n);

        /* We assume that the first authnStatement contains the data we want. */
        authnStatement = LASSO_SAML2_AUTHN_STATEMENT(assertion->AuthnStatement->data);

        if(!authnStatement) {
            ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
                          "No AuthnStatement found in the current assertion.");
            lasso_logout_destroy(logout);
            return HTTP_INTERNAL_SERVER_ERROR;
        }

        if(authnStatement->SessionIndex) {
            request = LASSO_SAMLP2_LOGOUT_REQUEST(profile->request);
            request->SessionIndex = g_strdup(authnStatement->SessionIndex);
        }
    }


    /* Set the RelayState parameter to the return url (if we have one). */
    if(return_to) {
        profile->msg_relayState = g_strdup(return_to);
    }

    /* Serialize the request message into a url which we can redirect to. */
    res = lasso_logout_build_request_msg(logout);
    if(res != 0) {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
                      "Unable to serialize lasso logout message."
                      " Lasso error: [%i] %s", res, lasso_strerror(res));

        lasso_logout_destroy(logout);
        return HTTP_INTERNAL_SERVER_ERROR;
    }

    /* Set the redirect url. */
    redirect_to = apr_pstrdup(r->pool, LASSO_PROFILE(logout)->msg_url);

    /* Check if the lasso library added the RelayState. If lasso didn't add
     * a RelayState parameter, then we add one ourself. This should hopefully
     * be removed in the future.
     */
    if(return_to != NULL
       && strstr(redirect_to, "&RelayState=") == NULL
       && strstr(redirect_to, "?RelayState=") == NULL) {
        /* The url didn't contain the relaystate parameter. */
        redirect_to = apr_pstrcat(
            r->pool, redirect_to, "&RelayState=",
            am_urlencode(r->pool, return_to),
            NULL
            );
    }

    apr_table_setn(r->headers_out, "Location", redirect_to);

    lasso_logout_destroy(logout);

    /* Redirect (without including POST data if this was a POST request. */
    return HTTP_SEE_OTHER;
}