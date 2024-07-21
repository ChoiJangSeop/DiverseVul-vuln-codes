int oidc_oauth_check_userid(request_rec *r, oidc_cfg *c) {

	/* check if this is a sub-request or an initial request */
	if (!ap_is_initial_req(r)) {

		if (r->main != NULL)
			r->user = r->main->user;
		else if (r->prev != NULL)
			r->user = r->prev->user;

		if (r->user != NULL) {

			/* this is a sub-request and we have a session */
			oidc_debug(r,
					"recycling user '%s' from initial request for sub-request",
					r->user);

			return OK;
		}

		/* check if this is a request for the public (encryption) keys */
	} else if ((c->redirect_uri != NULL)
			&& (oidc_util_request_matches_url(r, c->redirect_uri))) {

		if (oidc_util_request_has_parameter(r, "jwks")) {

			return oidc_handle_jwks(r, c);

		}

	}

	/* we don't have a session yet */

	/* get the bearer access token from the Authorization header */
	const char *access_token = NULL;
	if (oidc_oauth_get_bearer_token(r, &access_token) == FALSE)
		return oidc_oauth_return_www_authenticate(r, "invalid_request",
				"No bearer token found in the request");

	/* validate the obtained access token against the OAuth AS validation endpoint */
	json_t *token = NULL;
	char *s_token = NULL;

	/* check if an introspection endpoint is set */
	if (c->oauth.introspection_endpoint_url != NULL) {
		/* we'll validate the token remotely */
		if (oidc_oauth_resolve_access_token(r, c, access_token, &token,
				&s_token) == FALSE)
			return oidc_oauth_return_www_authenticate(r, "invalid_token",
					"Reference token could not be introspected");
	} else {
		/* no introspection endpoint is set, assume the token is a JWT and validate it locally */
		if (oidc_oauth_validate_jwt_access_token(r, c, access_token, &token,
				&s_token) == FALSE)
			return oidc_oauth_return_www_authenticate(r, "invalid_token",
					"JWT token could not be validated");
	}

	/* check that we've got something back */
	if (token == NULL) {
		oidc_error(r, "could not resolve claims (token == NULL)");
		return oidc_oauth_return_www_authenticate(r, "invalid_token",
				"No claims could be parsed from the token");
	}

	/* store the parsed token (cq. the claims from the response) in the request state so it can be accessed by the authz routines */
	oidc_request_state_set(r, OIDC_CLAIMS_SESSION_KEY, (const char *) s_token);

	/* set the REMOTE_USER variable */
	if (oidc_oauth_set_remote_user(r, c, token) == FALSE) {
		oidc_error(r,
				"remote user could not be set, aborting with HTTP_UNAUTHORIZED");
		return oidc_oauth_return_www_authenticate(r, "invalid_token",
				"Could not set remote user");
	}

	/* set the user authentication HTTP header if set and required */
	char *authn_header = oidc_cfg_dir_authn_header(r);
	int pass_headers = oidc_cfg_dir_pass_info_in_headers(r);
	int pass_envvars = oidc_cfg_dir_pass_info_in_envvars(r);

	if ((r->user != NULL) && (authn_header != NULL)) {
		oidc_debug(r, "setting authn header (%s) to: %s", authn_header,
				r->user);
		apr_table_set(r->headers_in, authn_header, r->user);
	}

	/* set the resolved claims in the HTTP headers for the target application */
	oidc_util_set_app_infos(r, token, c->claim_prefix, c->claim_delimiter,
			pass_headers, pass_envvars);

	/* set the access_token in the app headers */
	if (access_token != NULL) {
		oidc_util_set_app_info(r, "access_token", access_token,
				OIDC_DEFAULT_HEADER_PREFIX, pass_headers, pass_envvars);
	}

	/* free JSON resources */
	json_decref(token);

	return OK;
}