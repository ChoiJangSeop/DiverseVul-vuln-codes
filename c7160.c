e_ews_backend_ref_connection_sync (EEwsBackend *backend,
				   ESourceAuthenticationResult *result,
                                   GCancellable *cancellable,
                                   GError **error)
{
	EEwsConnection *connection = NULL;
	ESourceAuthenticationResult local_result;
	CamelEwsSettings *settings;
	gchar *hosturl;
	gboolean success;

	g_return_val_if_fail (E_IS_EWS_BACKEND (backend), NULL);

	g_mutex_lock (&backend->priv->connection_lock);
	if (backend->priv->connection != NULL)
		connection = g_object_ref (backend->priv->connection);
	g_mutex_unlock (&backend->priv->connection_lock);

	/* If we already have an authenticated
	 * connection object, just return that. */
	if (connection != NULL || !backend->priv->credentials)
		return connection;

	settings = ews_backend_get_settings (backend);
	hosturl = camel_ews_settings_dup_hosturl (settings);
	connection = e_ews_connection_new_full (e_backend_get_source (E_BACKEND (backend)), hosturl, settings, FALSE);
	g_free (hosturl);

	e_binding_bind_property (
		backend, "proxy-resolver",
		connection, "proxy-resolver",
		G_BINDING_SYNC_CREATE);

	local_result = e_ews_connection_try_credentials_sync (connection, backend->priv->credentials, cancellable, error);
	if (result)
		*result = local_result;

	success = local_result == E_SOURCE_AUTHENTICATION_ACCEPTED;

	if (success) {
		g_mutex_lock (&backend->priv->connection_lock);
		if (backend->priv->connection != NULL)
			g_object_unref (backend->priv->connection);
		backend->priv->connection = g_object_ref (connection);
		g_mutex_unlock (&backend->priv->connection_lock);
	} else {
		g_object_unref (connection);
		connection = NULL;
	}

	return connection;
}