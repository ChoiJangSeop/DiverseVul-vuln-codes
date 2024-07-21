get_settings (NMExportedConnection *exported)
{
	NMConnection *connection;
	GHashTable *settings;

	connection = nm_exported_connection_get_connection (exported);

	utils_fill_connection_certs (connection);
	settings = nm_connection_to_hash (connection);
	utils_clear_filled_connection_certs (connection);

	return settings;
}