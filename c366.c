add_connection_real (NMAGConfSettings *self, NMAGConfConnection *connection)
{
	NMAGConfSettingsPrivate *priv = NMA_GCONF_SETTINGS_GET_PRIVATE (self);

	if (connection) {
		priv->connections = g_slist_prepend (priv->connections, connection);
		g_signal_connect (connection, "new-secrets-requested",
					   G_CALLBACK (connection_new_secrets_requested_cb),
					   self);

		g_signal_connect (connection, "removed", G_CALLBACK (connection_removed), self);
		nm_settings_signal_new_connection (NM_SETTINGS (self),
									NM_EXPORTED_CONNECTION (connection));
	}
}