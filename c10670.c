static int oidc_session_redirect_parent_window_to_logout(request_rec *r,
		oidc_cfg *c) {

	oidc_debug(r, "enter");

	char *java_script = apr_psprintf(r->pool,
			"    <script type=\"text/javascript\">\n"
			"      window.top.location.href = '%s?session=logout';\n"
			"    </script>\n", oidc_get_redirect_uri(r, c));

	return oidc_util_html_send(r, "Redirecting...", java_script, NULL, NULL,
			OK);
}