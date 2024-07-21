static int oidc_handle_logout(request_rec *r, oidc_cfg *c,
		oidc_session_t *session) {

	oidc_provider_t *provider = NULL;
	/* pickup the command or URL where the user wants to go after logout */
	char *url = NULL;
	oidc_util_get_request_parameter(r, OIDC_REDIRECT_URI_REQUEST_LOGOUT, &url);

	oidc_debug(r, "enter (url=%s)", url);

	if (oidc_is_front_channel_logout(url)) {
		return oidc_handle_logout_request(r, c, session, url);
	} else if (oidc_is_back_channel_logout(url)) {
		return oidc_handle_logout_backchannel(r, c);
	}

	if ((url == NULL) || (apr_strnatcmp(url, "") == 0)) {

		url = c->default_slo_url;

	} else {

		/* do input validation on the logout parameter value */

		const char *error_description = NULL;
		apr_uri_t uri;

		if (apr_uri_parse(r->pool, url, &uri) != APR_SUCCESS) {
			const char *error_description = apr_psprintf(r->pool,
					"Logout URL malformed: %s", url);
			oidc_error(r, "%s", error_description);
			return oidc_util_html_send_error(r, c->error_template,
					"Malformed URL", error_description,
					HTTP_INTERNAL_SERVER_ERROR);

		}

		const char *c_host = oidc_get_current_url_host(r);
		if ((uri.hostname != NULL)
				&& ((strstr(c_host, uri.hostname) == NULL)
						|| (strstr(uri.hostname, c_host) == NULL))) {
			error_description =
					apr_psprintf(r->pool,
							"logout value \"%s\" does not match the hostname of the current request \"%s\"",
							apr_uri_unparse(r->pool, &uri, 0), c_host);
			oidc_error(r, "%s", error_description);
			return oidc_util_html_send_error(r, c->error_template,
					"Invalid Request", error_description,
					HTTP_INTERNAL_SERVER_ERROR);
		}

		/* validate the URL to prevent HTTP header splitting */
		if (((strstr(url, "\n") != NULL) || strstr(url, "\r") != NULL)) {
			error_description =
					apr_psprintf(r->pool,
							"logout value \"%s\" contains illegal \"\n\" or \"\r\" character(s)",
							url);
			oidc_error(r, "%s", error_description);
			return oidc_util_html_send_error(r, c->error_template,
					"Invalid Request", error_description,
					HTTP_INTERNAL_SERVER_ERROR);
		}
	}

	oidc_get_provider_from_session(r, c, session, &provider);

	if ((provider != NULL) && (provider->end_session_endpoint != NULL)) {

		const char *id_token_hint = oidc_session_get_idtoken(r, session);

		char *logout_request = apr_pstrdup(r->pool,
				provider->end_session_endpoint);
		if (id_token_hint != NULL) {
			logout_request = apr_psprintf(r->pool, "%s%sid_token_hint=%s",
					logout_request, strchr(logout_request ? logout_request : "",
							OIDC_CHAR_QUERY) != NULL ?
									OIDC_STR_AMP :
									OIDC_STR_QUERY,
									oidc_util_escape_string(r, id_token_hint));
		}

		if (url != NULL) {
			logout_request = apr_psprintf(r->pool,
					"%s%spost_logout_redirect_uri=%s", logout_request,
					strchr(logout_request ? logout_request : "",
							OIDC_CHAR_QUERY) != NULL ?
									OIDC_STR_AMP :
									OIDC_STR_QUERY,
									oidc_util_escape_string(r, url));
		}
		url = logout_request;
	}

	return oidc_handle_logout_request(r, c, session, url);
}