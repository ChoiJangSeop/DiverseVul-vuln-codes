read_connections (NMAGConfSettings *settings)
{
	NMAGConfSettingsPrivate *priv = NMA_GCONF_SETTINGS_GET_PRIVATE (settings);
	GSList *dir_list;
	GSList *iter;

	dir_list = nm_gconf_get_all_connections (priv->client);
	if (!dir_list)
		return;

	for (iter = dir_list; iter; iter = iter->next) {
		char *dir = (char *) iter->data;

		add_connection_real (settings, nma_gconf_connection_new (priv->client, dir));
		g_free (dir);
	}

	g_slist_free (dir_list);
	priv->connections = g_slist_reverse (priv->connections);
}