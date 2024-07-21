dispose (GObject *object)
{
	NMAGConfSettingsPrivate *priv = NMA_GCONF_SETTINGS_GET_PRIVATE (object);

	if (priv->disposed)
		return;

	priv->disposed = TRUE;

	g_hash_table_destroy (priv->pending_changes);

	if (priv->read_connections_id) {
		g_source_remove (priv->read_connections_id);
		priv->read_connections_id = 0;
	}

	gconf_client_notify_remove (priv->client, priv->conf_notify_id);
	gconf_client_remove_dir (priv->client, GCONF_PATH_CONNECTIONS, NULL);

	g_slist_foreach (priv->connections, (GFunc) g_object_unref, NULL);
	g_slist_free (priv->connections);

	g_object_unref (priv->client);

	G_OBJECT_CLASS (nma_gconf_settings_parent_class)->dispose (object);
}