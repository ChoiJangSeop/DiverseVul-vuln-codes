cd_device_db_load (CdDeviceDb *ddb,
		    const gchar *filename,
		    GError  **error)
{
	CdDeviceDbPrivate *priv = GET_PRIVATE (ddb);
	const gchar *statement;
	gchar *error_msg = NULL;
	gint rc;
	g_autofree gchar *path = NULL;

	g_return_val_if_fail (CD_IS_DEVICE_DB (ddb), FALSE);
	g_return_val_if_fail (priv->db == NULL, FALSE);

	/* ensure the path exists */
	path = g_path_get_dirname (filename);
	if (!cd_main_mkdir_with_parents (path, error))
		return FALSE;

	g_debug ("CdDeviceDb: trying to open database '%s'", filename);
	g_info ("Using device database file %s", filename);
	rc = sqlite3_open (filename, &priv->db);
	if (rc != SQLITE_OK) {
		g_set_error (error,
			     CD_CLIENT_ERROR,
			     CD_CLIENT_ERROR_INTERNAL,
			     "Can't open database: %s\n",
			     sqlite3_errmsg (priv->db));
		sqlite3_close (priv->db);
		return FALSE;
	}

	/* we don't need to keep doing fsync */
	sqlite3_exec (priv->db, "PRAGMA synchronous=OFF",
		      NULL, NULL, NULL);

	/* check devices */
	rc = sqlite3_exec (priv->db, "SELECT * FROM devices LIMIT 1",
			   NULL, NULL, &error_msg);
	if (rc != SQLITE_OK) {
		g_debug ("CdDeviceDb: creating table to repair: %s", error_msg);
		sqlite3_free (error_msg);
		statement = "CREATE TABLE devices ("
			    "device_id TEXT PRIMARY KEY,"
			    "device TEXT);";
		sqlite3_exec (priv->db, statement, NULL, NULL, NULL);
	}

	/* check properties version 2 */
	rc = sqlite3_exec (priv->db, "SELECT * FROM properties_v2 LIMIT 1",
			   NULL, NULL, &error_msg);
	if (rc != SQLITE_OK) {
		statement = "CREATE TABLE properties_v2 ("
			    "device_id TEXT,"
			    "property TEXT,"
			    "value TEXT,"
			    "PRIMARY KEY (device_id, property));";
		sqlite3_exec (priv->db, statement, NULL, NULL, NULL);
	}
	return TRUE;
}