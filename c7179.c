e_ews_config_utils_open_connection_for (ESource *source,
                                        CamelEwsSettings *ews_settings,
					const gchar *connect_url,
					EEwsConfigUtilTryCredentialsFunc try_credentials_func,
					gpointer user_data,
                                        GCancellable *cancellable,
                                        GError **perror)
{
	EEwsConnection *conn = NULL;
	CamelNetworkSettings *network_settings;
	GError *local_error = NULL;

	g_return_val_if_fail (source != NULL, NULL);
	g_return_val_if_fail (ews_settings != NULL, NULL);

	network_settings = CAMEL_NETWORK_SETTINGS (ews_settings);

	/* use the one from mailer, if there, otherwise open new */
	conn = e_ews_connection_find (
		connect_url && *connect_url ? connect_url : camel_ews_settings_get_hosturl (ews_settings),
		camel_network_settings_get_user (network_settings));
	if (conn) {
		if (try_credentials_func &&
		    try_credentials_func (conn, NULL, user_data, cancellable, perror) != E_SOURCE_AUTHENTICATION_ACCEPTED) {
			g_clear_object (&conn);
		}
		return conn;
	}

	while (!conn && !g_cancellable_is_cancelled (cancellable) && !local_error) {
		if (e_ews_connection_utils_get_without_password (ews_settings)) {
			ESourceAuthenticationResult result;
			gchar *hosturl;

			hosturl = camel_ews_settings_dup_hosturl (ews_settings);
			conn = e_ews_connection_new (source, connect_url && *connect_url ? connect_url : hosturl, ews_settings);
			g_free (hosturl);

			e_ews_connection_update_credentials (conn, NULL);

			if (try_credentials_func)
				result = try_credentials_func (conn, NULL, user_data, cancellable, &local_error);
			else
				result = e_ews_connection_try_credentials_sync (conn, NULL, cancellable, &local_error);

			if (result != E_SOURCE_AUTHENTICATION_ACCEPTED) {
				g_clear_object (&conn);
				if (result != E_SOURCE_AUTHENTICATION_REJECTED || local_error)
					break;
			}
		}

		if (!conn) {
			EShell *shell;
			TryCredentialsData data;

			e_ews_connection_utils_force_off_ntlm_auth_check ();
			g_clear_error (&local_error);

			shell = e_shell_get_default ();

			data.ews_settings = g_object_ref (ews_settings);
			data.connect_url = connect_url && *connect_url ? connect_url : NULL;
			data.try_credentials_func = try_credentials_func;
			data.user_data = user_data;
			data.conn = NULL;

			e_credentials_prompter_loop_prompt_sync (e_shell_get_credentials_prompter (shell),
				source, E_CREDENTIALS_PROMPTER_PROMPT_FLAG_ALLOW_SOURCE_SAVE,
				ews_config_utils_try_credentials_sync, &data, cancellable, &local_error);

			if (data.conn)
				conn = g_object_ref (data.conn);

			g_clear_object (&data.ews_settings);
			g_clear_object (&data.conn);
		}
	}

	if (local_error)
		g_propagate_error (perror, local_error);

	return conn;
}