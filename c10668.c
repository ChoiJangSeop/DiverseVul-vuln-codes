apr_byte_t oidc_post_preserve_javascript(request_rec *r, const char *location,
		char **javascript, char **javascript_method) {

	if (oidc_cfg_dir_preserve_post(r) == 0)
		return FALSE;

	oidc_debug(r, "enter");

	oidc_cfg *cfg = ap_get_module_config(r->server->module_config,
			&auth_openidc_module);

	const char *method = oidc_original_request_method(r, cfg, FALSE);

	if (apr_strnatcmp(method, OIDC_METHOD_FORM_POST) != 0)
		return FALSE;

	/* read the parameters that are POST-ed to us */
	apr_table_t *params = apr_table_make(r->pool, 8);
	if (oidc_util_read_post_params(r, params, FALSE, NULL) == FALSE) {
		oidc_error(r, "something went wrong when reading the POST parameters");
		return FALSE;
	}

	const apr_array_header_t *arr = apr_table_elts(params);
	const apr_table_entry_t *elts = (const apr_table_entry_t*) arr->elts;
	int i;
	char *json = "";
	for (i = 0; i < arr->nelts; i++) {
		json = apr_psprintf(r->pool, "%s'%s': '%s'%s", json,
				oidc_util_escape_string(r, elts[i].key),
				oidc_util_escape_string(r, elts[i].val),
				i < arr->nelts - 1 ? "," : "");
	}
	json = apr_psprintf(r->pool, "{ %s }", json);

	const char *jmethod = "preserveOnLoad";
	const char *jscript =
			apr_psprintf(r->pool,
					"    <script type=\"text/javascript\">\n"
					"      function %s() {\n"
					"        sessionStorage.setItem('mod_auth_openidc_preserve_post_params', JSON.stringify(%s));\n"
					"        %s"
					"      }\n"
					"    </script>\n", jmethod, json,
					location ?
							apr_psprintf(r->pool, "window.location='%s';\n",
									location) :
									"");
	if (location == NULL) {
		if (javascript_method)
			*javascript_method = apr_pstrdup(r->pool, jmethod);
		if (javascript)
			*javascript = apr_pstrdup(r->pool, jscript);
	} else {
		oidc_util_html_send(r, "Preserving...", jscript, jmethod,
				"<p>Preserving...</p>", OK);
	}

	return TRUE;
}