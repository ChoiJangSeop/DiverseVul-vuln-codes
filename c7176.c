mail_config_ews_autodiscover_sync (ECredentialsPrompter *prompter,
				   ESource *source,
				   const ENamedParameters *credentials,
				   gboolean *out_authenticated,
				   gpointer user_data,
				   GCancellable *cancellable,
				   GError **error)
{
	AsyncContext *async_context = user_data;
	GError *local_error = NULL;
	gboolean res = TRUE;

	e_ews_autodiscover_ws_url_sync (source,
		async_context->ews_settings, async_context->email_address,
		credentials && e_named_parameters_get (credentials, E_SOURCE_CREDENTIAL_PASSWORD) ?
		e_named_parameters_get (credentials, E_SOURCE_CREDENTIAL_PASSWORD) : "",
		cancellable, &local_error);

	if (local_error == NULL) {
		*out_authenticated = TRUE;
	} else if (g_error_matches (local_error, SOUP_HTTP_ERROR, SOUP_STATUS_UNAUTHORIZED)) {
		*out_authenticated = FALSE;
		g_error_free (local_error);
	} else {
		res = FALSE;
		g_propagate_error (error, local_error);
	}

	return res;
}