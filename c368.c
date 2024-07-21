connection_changes_done (gpointer data)
{
	ConnectionChangedInfo *info = (ConnectionChangedInfo *) data;
	NMAGConfSettingsPrivate *priv = NMA_GCONF_SETTINGS_GET_PRIVATE (info->settings);
	NMAGConfConnection *connection;

	connection = nma_gconf_settings_get_by_path (info->settings, info->path);
	if (!connection) {
		/* New connection */
		connection = nma_gconf_connection_new (priv->client, info->path);
		add_connection_real (info->settings, connection);
	} else {
		if (gconf_client_dir_exists (priv->client, info->path, NULL)) {
			/* Updated connection */
			if (!nma_gconf_connection_changed (connection))
				priv->connections = g_slist_remove (priv->connections, connection);
		}
	}

	g_hash_table_remove (priv->pending_changes, info->path);

	return FALSE;
}