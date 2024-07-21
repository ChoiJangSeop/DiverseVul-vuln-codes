static int oidc_check_userid_openidc(request_rec *r, oidc_cfg *c) {

	if (c->redirect_uri == NULL) {
		oidc_error(r,
				"configuration error: the authentication type is set to \"openid-connect\" but OIDCRedirectURI has not been set");
		return HTTP_INTERNAL_SERVER_ERROR;
	}

	/* check if this is a sub-request or an initial request */
	if (ap_is_initial_req(r)) {

		int rc = OK;

		/* load the session from the request state; this will be a new "empty" session if no state exists */
		oidc_session_t *session = NULL;
		oidc_session_load(r, &session);

		/* see if the initial request is to the redirect URI; this handles potential logout too */
		if (oidc_util_request_matches_url(r, c->redirect_uri)) {

			/* handle request to the redirect_uri */
			rc = oidc_handle_redirect_uri_request(r, c, session);

			/* free resources allocated for the session */
			oidc_session_free(r, session);

			return rc;

		/* initial request to non-redirect URI, check if we have an existing session */
		} else if (session->remote_user != NULL) {

			/* set the user in the main request for further (incl. sub-request) processing */
			r->user = (char *) session->remote_user;

			/* this is initial request and we already have a session */
			rc = oidc_handle_existing_session(r, c, session);

			/* free resources allocated for the session */
			oidc_session_free(r, session);

			/* strip any cookies that we need to */
			oidc_strip_cookies(r);

			return rc;
		}

		/* free resources allocated for the session */
		oidc_session_free(r, session);

		/*
		 * else: initial request, we have no session and it is not an authorization or
		 *       discovery response: just hit the default flow for unauthenticated users
		 */
	} else {

		/* not an initial request, try to recycle what we've already established in the main request */
		if (r->main != NULL)
			r->user = r->main->user;
		else if (r->prev != NULL)
			r->user = r->prev->user;

		if (r->user != NULL) {

			/* this is a sub-request and we have a session (headers will have been scrubbed and set already) */
			oidc_debug(r,
					"recycling user '%s' from initial request for sub-request",
					r->user);

			/*
			 * apparently request state can get lost in sub-requests, so let's see
			 * if we need to restore id_token and/or claims from the session cache
			 */
			const char *s_id_token = oidc_request_state_get(r,
					OIDC_IDTOKEN_CLAIMS_SESSION_KEY);
			if (s_id_token == NULL) {

				oidc_session_t *session = NULL;
				oidc_session_load(r, &session);

				oidc_copy_tokens_to_request_state(r, session, NULL, NULL);

				/* free resources allocated for the session */
				oidc_session_free(r, session);
			}

			/* strip any cookies that we need to */
			oidc_strip_cookies(r);

			return OK;
		}
		/*
		 * else: not initial request, but we could not find a session, so:
		 * just hit the default flow for unauthenticated users
		 */
	}

	/* find out which action we need to take when encountering an unauthenticated request */
	switch (oidc_dir_cfg_unauth_action(r)) {
		case OIDC_UNAUTH_RETURN410:
			return HTTP_GONE;
		case OIDC_UNAUTH_RETURN401:
			return HTTP_UNAUTHORIZED;
		case OIDC_UNAUTH_PASS:
			r->user = "";
			return OK;
		case OIDC_UNAUTH_AUTHENTICATE:
			/* if this is a Javascript path we won't redirect the user and create a state cookie */
			if (apr_table_get(r->headers_in, "X-Requested-With") != NULL)
				return HTTP_UNAUTHORIZED;
			break;
	}

	/* else: no session (regardless of whether it is main or sub-request), go and authenticate the user */
	return oidc_authenticate_user(r, c, NULL, oidc_get_current_url(r), NULL,
			NULL, NULL, NULL);
}