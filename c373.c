nma_gconf_connection_new (GConfClient *client, const char *conf_dir)
{
	NMConnection *connection;
	NMAGConfConnection *gconf_connection;

	g_return_val_if_fail (GCONF_IS_CLIENT (client), NULL);
	g_return_val_if_fail (conf_dir != NULL, NULL);

	/* retrieve GConf data */
	connection = nm_gconf_read_connection (client, conf_dir);
	if (connection) {
		gconf_connection = nma_gconf_connection_new_from_connection (client, conf_dir, connection);
		g_object_unref (connection);
	} else {
		nm_warning ("No connection read from GConf at %s.", conf_dir);
		gconf_connection = NULL;
	}
	
	return gconf_connection;
}