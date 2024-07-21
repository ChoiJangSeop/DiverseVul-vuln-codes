add_connection_real (NMAGConfSettings *self, NMAGConfConnection *connection)
{
	NMAGConfSettingsPrivate *priv = NMA_GCONF_SETTINGS_GET_PRIVATE (self);

	g_return_if_fail (connection != NULL);

	priv->connections = g_slist_prepend (priv->connections, connection);
	g_signal_connect (connection, "new-secrets-requested",
				   G_CALLBACK (connection_new_secrets_requested_cb),
				   self);

	g_signal_connect (connection, "removed", G_CALLBACK (connection_removed), self);

	/* Export the connection over dbus if requested */
	if (priv->bus) {
		nm_exported_connection_register_object (NM_EXPORTED_CONNECTION (connection),
		                                        NM_CONNECTION_SCOPE_USER,
		                                        priv->bus);
		dbus_g_connection_unref (priv->bus);
	}

	nm_settings_signal_new_connection (NM_SETTINGS (self), NM_EXPORTED_CONNECTION (connection));
}