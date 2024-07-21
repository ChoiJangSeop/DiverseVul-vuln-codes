ews_config_lookup_worker_run (EConfigLookupWorker *lookup_worker,
			      EConfigLookup *config_lookup,
			      const ENamedParameters *params,
			      ENamedParameters **out_restart_params,
			      GCancellable *cancellable,
			      GError **error)
{
	CamelEwsSettings *ews_settings = NULL;
	ESource *source;
	const gchar *email_address;
	const gchar *password;
	const gchar *extension_name;

	g_return_if_fail (E_IS_EWS_CONFIG_LOOKUP (lookup_worker));
	g_return_if_fail (E_IS_CONFIG_LOOKUP (config_lookup));
	g_return_if_fail (params != NULL);

	email_address = e_named_parameters_get (params, E_CONFIG_LOOKUP_PARAM_EMAIL_ADDRESS);

	if (!email_address || !*email_address)
		return;

	if (!e_named_parameters_exists (params, E_CONFIG_LOOKUP_PARAM_PASSWORD)) {
		g_set_error (error, E_CONFIG_LOOKUP_WORKER_ERROR, E_CONFIG_LOOKUP_WORKER_ERROR_REQUIRES_PASSWORD,
			_("Requires user password to continue"));
		return;
	}

	ews_settings = g_object_new (CAMEL_TYPE_EWS_SETTINGS, NULL);
	camel_ews_settings_set_email (ews_settings, email_address);

	extension_name = e_source_camel_get_extension_name ("ews");
	source = e_config_lookup_get_source (config_lookup, E_CONFIG_LOOKUP_SOURCE_COLLECTION);
	if (source && e_source_has_extension (source, extension_name)) {
		ESourceCamel *camel_extension;
		CamelSettings *settings;

		camel_extension = e_source_get_extension (source, extension_name);
		settings = e_source_camel_get_settings (camel_extension);

		/* Copy only the host url */
		if (CAMEL_IS_EWS_SETTINGS (settings))
			camel_ews_settings_set_hosturl (ews_settings, camel_ews_settings_get_hosturl (CAMEL_EWS_SETTINGS (settings)));
	}

	password = e_named_parameters_get (params, E_CONFIG_LOOKUP_PARAM_PASSWORD);

	if (password) {
		const gchar *servers;

		if (e_ews_autodiscover_ws_url_sync (source, ews_settings, email_address, password, cancellable, NULL)) {
			ews_config_lookup_worker_result_from_settings (lookup_worker, config_lookup, email_address, ews_settings, params);
		}

		servers = e_named_parameters_get (params, E_CONFIG_LOOKUP_PARAM_SERVERS);

		if (servers && *servers) {
			gchar **servers_strv;
			gint ii;

			servers_strv = g_strsplit (servers, ";", 0);

			for (ii = 0; servers_strv && servers_strv[ii] && !g_cancellable_is_cancelled (cancellable); ii++) {
				const gchar *server = servers_strv[ii];
				gchar *tmp = NULL;

				if (*server && !strstr (server, "://")) {
					tmp = g_strconcat ("https://", server, "/EWS/Exchange.asmx", NULL);
					server = tmp;
				}

				camel_ews_settings_set_hosturl (ews_settings, server);

				if (e_ews_autodiscover_ws_url_sync (source, ews_settings, email_address, password, cancellable, NULL)) {
					ews_config_lookup_worker_result_from_settings (lookup_worker, config_lookup, email_address, ews_settings, params);
				}

				g_free (tmp);
			}

			g_strfreev (servers_strv);
		}

		if (out_restart_params)
			*out_restart_params = e_named_parameters_new_clone (params);
	}

	g_clear_object (&ews_settings);
}