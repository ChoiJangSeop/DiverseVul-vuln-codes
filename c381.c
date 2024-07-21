add_one_setting (GHashTable *settings,
                 NMConnection *connection,
                 NMSetting *setting,
                 GError **error)
{
	GHashTable *secrets;

	g_return_val_if_fail (settings != NULL, FALSE);
	g_return_val_if_fail (connection != NULL, FALSE);
	g_return_val_if_fail (setting != NULL, FALSE);
	g_return_val_if_fail (error != NULL, FALSE);
	g_return_val_if_fail (*error == NULL, FALSE);

	utils_fill_connection_certs (connection);
	secrets = nm_setting_to_hash (setting);
	utils_clear_filled_connection_certs (connection);

	if (secrets) {
		g_hash_table_insert (settings, g_strdup (nm_setting_get_name (setting)), secrets);
	} else {
		g_set_error (error, NM_SETTINGS_ERROR, NM_SETTINGS_ERROR_INTERNAL_ERROR,
		             "%s.%d (%s): failed to hash setting '%s'.",
		             __FILE__, __LINE__, __func__, nm_setting_get_name (setting));
	}

	return secrets ? TRUE : FALSE;
}