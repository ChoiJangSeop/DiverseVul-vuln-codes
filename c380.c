get_8021x_secrets_cb (GtkDialog *dialog,
					  gint response,
					  gpointer user_data)
{
	NM8021xInfo *info = user_data;
	NMAGConfConnection *gconf_connection;
	NMConnection *connection = NULL;
	NMSetting *setting;
	GHashTable *settings_hash;
	GHashTable *secrets;
	GError *err = NULL;

	/* Got a user response, clear the NMActiveConnection destroy handler for
	 * this dialog since this function will now take over dialog destruction.
	 */
	g_object_weak_unref (G_OBJECT (info->active_connection), destroy_8021x_dialog, info);

	if (response != GTK_RESPONSE_OK) {
		g_set_error (&err, NM_SETTINGS_ERROR, NM_SETTINGS_ERROR_SECRETS_REQUEST_CANCELED,
		             "%s.%d (%s): canceled",
		             __FILE__, __LINE__, __func__);
		goto done;
	}

	connection = nma_wired_dialog_get_connection (info->dialog);
	if (!connection) {
		g_set_error (&err, NM_SETTINGS_ERROR, NM_SETTINGS_ERROR_INTERNAL_ERROR,
		             "%s.%d (%s): couldn't get connection from wired dialog.",
		             __FILE__, __LINE__, __func__);
		goto done;
	}

	setting = nm_connection_get_setting (connection, NM_TYPE_SETTING_802_1X);
	if (!setting) {
		g_set_error (&err, NM_SETTINGS_ERROR, NM_SETTINGS_ERROR_INVALID_CONNECTION,
					 "%s.%d (%s): requested setting '802-1x' didn't"
					 " exist in the connection.",
					 __FILE__, __LINE__, __func__);
		goto done;
	}

	utils_fill_connection_certs (NM_CONNECTION (connection));
	secrets = nm_setting_to_hash (setting);
	utils_clear_filled_connection_certs (NM_CONNECTION (connection));

	if (!secrets) {
		g_set_error (&err, NM_SETTINGS_ERROR, NM_SETTINGS_ERROR_INTERNAL_ERROR,
					 "%s.%d (%s): failed to hash setting '%s'.",
					 __FILE__, __LINE__, __func__, nm_setting_get_name (setting));
		goto done;
	}

	/* Returned secrets are a{sa{sv}}; this is the outer a{s...} hash that
	 * will contain all the individual settings hashes.
	 */
	settings_hash = g_hash_table_new_full (g_str_hash, g_str_equal,
										   g_free, (GDestroyNotify) g_hash_table_destroy);

	g_hash_table_insert (settings_hash, g_strdup (nm_setting_get_name (setting)), secrets);
	dbus_g_method_return (info->context, settings_hash);
	g_hash_table_destroy (settings_hash);

	/* Save the connection back to GConf _after_ hashing it, because
	 * saving to GConf might trigger the GConf change notifiers, resulting
	 * in the connection being read back in from GConf which clears secrets.
	 */
	gconf_connection = nma_gconf_settings_get_by_connection (info->applet->gconf_settings, connection);
	if (gconf_connection)
		nma_gconf_connection_save (gconf_connection);

done:
	if (err) {
		g_warning ("%s", err->message);
		dbus_g_method_return_error (info->context, err);
		g_error_free (err);
	}

	if (connection)
		nm_connection_clear_secrets (connection);

	destroy_8021x_dialog (info, NULL);
}