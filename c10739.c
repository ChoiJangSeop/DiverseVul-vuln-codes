static apr_byte_t oidc_validate_redirect_url(request_rec *r, oidc_cfg *c,
		const char *redirect_to_url, apr_byte_t restrict_to_host, char **err_str,
		char **err_desc) {
	apr_uri_t uri;
	const char *c_host = NULL;
	apr_hash_index_t *hi = NULL;
	size_t i = 0;
	char *url = apr_pstrndup(r->pool, redirect_to_url, OIDC_MAX_URL_LENGTH);

	// replace potentially harmful backslashes with forward slashes
	for (i = 0; i < strlen(url); i++)
		if (url[i] == '\\')
			url[i] = '/';

	if (apr_uri_parse(r->pool, url, &uri) != APR_SUCCESS) {
		*err_str = apr_pstrdup(r->pool, "Malformed URL");
		*err_desc = apr_psprintf(r->pool, "not a valid URL value: %s", url);
		oidc_error(r, "%s: %s", *err_str, *err_desc);
		return FALSE;
	}

	if (c->redirect_urls_allowed != NULL) {
		for (hi = apr_hash_first(NULL, c->redirect_urls_allowed); hi; hi =
				apr_hash_next(hi)) {
			apr_hash_this(hi, (const void**) &c_host, NULL, NULL);
			if (oidc_util_regexp_first_match(r->pool, url, c_host,
					NULL, err_str) == TRUE)
				break;
		}
		if (hi == NULL) {
			*err_str = apr_pstrdup(r->pool, "URL not allowed");
			*err_desc =
					apr_psprintf(r->pool,
							"value does not match the list of allowed redirect URLs: %s",
							url);
			oidc_error(r, "%s: %s", *err_str, *err_desc);
			return FALSE;
		}
	} else if ((uri.hostname != NULL) && (restrict_to_host == TRUE)) {
		c_host = oidc_get_current_url_host(r);
		if ((strstr(c_host, uri.hostname) == NULL)
				|| (strstr(uri.hostname, c_host) == NULL)) {
			*err_str = apr_pstrdup(r->pool, "Invalid Request");
			*err_desc =
					apr_psprintf(r->pool,
							"URL value \"%s\" does not match the hostname of the current request \"%s\"",
							apr_uri_unparse(r->pool, &uri, 0), c_host);
			oidc_error(r, "%s: %s", *err_str, *err_desc);
			return FALSE;
		}
	}

	if ((uri.hostname == NULL) && (strstr(url, "/") != url)) {
		*err_str = apr_pstrdup(r->pool, "Malformed URL");
		*err_desc =
				apr_psprintf(r->pool,
						"No hostname was parsed and it does not seem to be relative, i.e starting with '/': %s",
						url);
		oidc_error(r, "%s: %s", *err_str, *err_desc);
		return FALSE;
	} else if ((uri.hostname == NULL) && (strstr(url, "//") == url)) {
		*err_str = apr_pstrdup(r->pool, "Malformed URL");
		*err_desc = apr_psprintf(r->pool,
				"No hostname was parsed and starting with '//': %s", url);
		oidc_error(r, "%s: %s", *err_str, *err_desc);
		return FALSE;
	} else if ((uri.hostname == NULL) && (strstr(url, "/\\") == url)) {
		*err_str = apr_pstrdup(r->pool, "Malformed URL");
		*err_desc = apr_psprintf(r->pool,
				"No hostname was parsed and starting with '/\\': %s", url);
		oidc_error(r, "%s: %s", *err_str, *err_desc);
		return FALSE;
	}

	/* validate the URL to prevent HTTP header splitting */
	if (((strstr(url, "\n") != NULL) || strstr(url, "\r") != NULL)) {
		*err_str = apr_pstrdup(r->pool, "Invalid URL");
		*err_desc =
				apr_psprintf(r->pool,
						"URL value \"%s\" contains illegal \"\n\" or \"\r\" character(s)",
						url);
		oidc_error(r, "%s: %s", *err_str, *err_desc);
		return FALSE;
	}

	return TRUE;
}