ews_config_utils_try_credentials_sync (ECredentialsPrompter *prompter,
				       ESource *source,
				       const ENamedParameters *credentials,
				       gboolean *out_authenticated,
				       gpointer user_data,
				       GCancellable *cancellable,
				       GError **error)
{
	TryCredentialsData *data = user_data;
	ESourceAuthenticationResult auth_result;
	gchar *hosturl;
	gboolean res = TRUE;

	hosturl = camel_ews_settings_dup_hosturl (data->ews_settings);
	data->conn = e_ews_connection_new (source, data->connect_url ? data->connect_url : hosturl, data->ews_settings);
	g_free (hosturl);

	e_ews_connection_update_credentials (data->conn, credentials);

	if (data->try_credentials_func)
		auth_result = data->try_credentials_func (data->conn, credentials, data->user_data, cancellable, error);
	else
		auth_result = e_ews_connection_try_credentials_sync (data->conn, credentials, cancellable, error);

	if (auth_result == E_SOURCE_AUTHENTICATION_ACCEPTED) {
		*out_authenticated = TRUE;
	} else if (auth_result == E_SOURCE_AUTHENTICATION_REJECTED) {
		*out_authenticated = FALSE;
		g_clear_object (&data->conn);
		g_clear_error (error);
	} else {
		res = FALSE;
		g_clear_object (&data->conn);
	}

	return res;
}