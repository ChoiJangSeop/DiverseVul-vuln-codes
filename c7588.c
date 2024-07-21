static int am_handle_reply_common(request_rec *r, LassoLogin *login,
                                  char *relay_state, char *saml_response,
                                  bool is_paos)
{
    char *url;
    char *chr;
    const char *name_id;
    LassoSamlp2Response *response;
    LassoSaml2Assertion *assertion;
    const char *in_response_to;
    am_dir_cfg_rec *dir_cfg;
    am_cache_entry_t *session;
    int rc;
    const char *idp;

    url = am_reconstruct_url(r);
    chr = strchr(url, '?');
    if (! chr) {
        chr = strchr(url, ';');
    }
    if (chr) {
        *chr = '\0';
    }


    dir_cfg = am_get_dir_cfg(r);

    if(LASSO_PROFILE(login)->nameIdentifier == NULL) {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
                      "No acceptable name identifier found in"
                      " SAML 2.0 response.");
        lasso_login_destroy(login);
        return HTTP_BAD_REQUEST;
    }

    name_id = LASSO_SAML2_NAME_ID(LASSO_PROFILE(login)->nameIdentifier)
        ->content;

    response = LASSO_SAMLP2_RESPONSE(LASSO_PROFILE(login)->response);

    if (response->parent.Destination) {
        if (strcmp(response->parent.Destination, url)) {
            ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
                          "Invalid Destination on Response. Should be: %s",
                          url);
            lasso_login_destroy(login);
            return HTTP_BAD_REQUEST;
        }
    }

    if (g_list_length(response->Assertion) == 0) {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
                      "No Assertion in response.");
        lasso_login_destroy(login);
        return HTTP_BAD_REQUEST;
    }
    if (g_list_length(response->Assertion) > 1) {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
                      "More than one Assertion in response.");
        lasso_login_destroy(login);
        return HTTP_BAD_REQUEST;
    }
    assertion = g_list_first(response->Assertion)->data;
    if (!LASSO_IS_SAML2_ASSERTION(assertion)) {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
                      "Wrong type of Assertion node.");
        lasso_login_destroy(login);
        return HTTP_BAD_REQUEST;
    }

    rc = am_validate_subject(r, assertion, url);
    if (rc != OK) {
        lasso_login_destroy(login);
        return rc;
    }

    rc = am_validate_conditions(r, assertion,
        LASSO_PROVIDER(LASSO_PROFILE(login)->server)->ProviderID);

    if (rc != OK) {
        lasso_login_destroy(login);
        return rc;
    }

    in_response_to = response->parent.InResponseTo;


    if (!is_paos) {
        if(in_response_to != NULL) {
            /* This is SP-initiated login. Check that we have a cookie. */
            if(am_cookie_get(r) == NULL) {
                /* Missing cookie. */
                ap_log_rerror(APLOG_MARK, APLOG_WARNING, 0, r,
                              "User has disabled cookies, or has lost"
                              " the cookie before returning from the SAML2"
                              " login server.");
                if(dir_cfg->no_cookie_error_page != NULL) {
                    apr_table_setn(r->headers_out, "Location",
                                   dir_cfg->no_cookie_error_page);
                    lasso_login_destroy(login);
                    return HTTP_SEE_OTHER;
                } else {
                    /* Return 400 Bad Request when the user hasn't set a
                     * no-cookie error page.
                     */
                    lasso_login_destroy(login);
                    return HTTP_BAD_REQUEST;
                }
            }
        }
    }

    /* Check AuthnContextClassRef */
    rc = am_validate_authn_context_class_ref(r, assertion);
    if (rc != OK) {
        lasso_login_destroy(login);
        return rc;
    }

    /* Create a new session. */
    session = am_new_request_session(r);
    if(session == NULL) {
        ap_log_error(APLOG_MARK, APLOG_ERR, 0, NULL,
                    "am_new_request_session() failed");
        return HTTP_INTERNAL_SERVER_ERROR;
    }

    rc = add_attributes(session, r, name_id, assertion);
    if(rc != OK) {
        am_release_request_session(r, session);
        lasso_login_destroy(login);
        return rc;
    }

    /* If requested, save the IdP ProviderId */
    if(dir_cfg->idpattr != NULL) {
        idp = LASSO_PROFILE(login)->remote_providerID;
        if(idp != NULL) {
            rc = am_cache_env_append(session, dir_cfg->idpattr, idp);
            if(rc != OK) {
                am_release_request_session(r, session);
                lasso_login_destroy(login);
                return rc;
            }
        }
    }

    rc = lasso_login_accept_sso(login);
    if(rc < 0) {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r,
                      "Unable to accept SSO message."
                      " Lasso error: [%i] %s", rc, lasso_strerror(rc));
        am_release_request_session(r, session);
        lasso_login_destroy(login);
        return HTTP_INTERNAL_SERVER_ERROR;
    }


    /* Save the profile state. */
    rc = am_save_lasso_profile_state(r, session, LASSO_PROFILE(login),
                                     saml_response);
    if(rc != OK) {
        am_release_request_session(r, session);
        lasso_login_destroy(login);
        return rc;
    }

    /* Mark user as logged in. */
    session->logged_in = 1;

    am_release_request_session(r, session);
    lasso_login_destroy(login);


    /* No RelayState - we don't know what to do. Use default login path. */
    if(relay_state == NULL || strlen(relay_state) == 0) {
       dir_cfg = am_get_dir_cfg(r);
       apr_table_setn(r->headers_out, "Location", dir_cfg->login_path);
       return HTTP_SEE_OTHER;
    }

    rc = am_urldecode(relay_state);
    if (rc != OK) {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, rc, r,
                      "Could not urldecode RelayState value.");
        return HTTP_BAD_REQUEST;
    }

    /* Check for bad characters in RelayState. */
    rc = am_check_url(r, relay_state);
    if (rc != OK) {
        return rc;
    }

    apr_table_setn(r->headers_out, "Location",
                   relay_state);

    /* HTTP_SEE_OTHER should be a redirect where the browser doesn't repeat
     * the POST data to the new page.
     */
    return HTTP_SEE_OTHER;
}