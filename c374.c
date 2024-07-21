nma_gconf_connection_changed (NMAGConfConnection *self)
{
	NMAGConfConnectionPrivate *priv;
	GHashTable *settings;
	NMConnection *wrapped_connection;
	NMConnection *gconf_connection;
	GHashTable *new_settings;
	GError *error = NULL;

	g_return_val_if_fail (NMA_IS_GCONF_CONNECTION (self), FALSE);

	priv = NMA_GCONF_CONNECTION_GET_PRIVATE (self);
	wrapped_connection = nm_exported_connection_get_connection (NM_EXPORTED_CONNECTION (self));

	gconf_connection = nm_gconf_read_connection (priv->client, priv->dir);
	if (!gconf_connection) {
		g_warning ("No connection read from GConf at %s.", priv->dir);
		goto invalid;
	}

	utils_fill_connection_certs (gconf_connection);
	if (!nm_connection_verify (gconf_connection, &error)) {
		utils_clear_filled_connection_certs (gconf_connection);
		g_warning ("%s: Invalid connection %s: '%s' / '%s' invalid: %d",
		           __func__, priv->dir,
		           g_type_name (nm_connection_lookup_setting_type_by_quark (error->domain)),
		           error->message, error->code);
		goto invalid;
	}
	utils_clear_filled_connection_certs (gconf_connection);

	/* Ignore the GConf update if nothing changed */
	if (   nm_connection_compare (wrapped_connection, gconf_connection, NM_SETTING_COMPARE_FLAG_EXACT)
	    && nm_gconf_compare_private_connection_values (wrapped_connection, gconf_connection))
		return TRUE;

	/* Update private values to catch any certificate path changes */
	nm_gconf_copy_private_connection_values (wrapped_connection, gconf_connection);

	utils_fill_connection_certs (gconf_connection);
	new_settings = nm_connection_to_hash (gconf_connection);
	utils_clear_filled_connection_certs (gconf_connection);

	if (!nm_connection_replace_settings (wrapped_connection, new_settings, &error)) {
		utils_clear_filled_connection_certs (wrapped_connection);
		g_hash_table_destroy (new_settings);

		g_warning ("%s: '%s' / '%s' invalid: %d",
		           __func__,
		           error ? g_type_name (nm_connection_lookup_setting_type_by_quark (error->domain)) : "(none)",
		           (error && error->message) ? error->message : "(none)",
		           error ? error->code : -1);
		goto invalid;
	}
	g_object_unref (gconf_connection);
	g_hash_table_destroy (new_settings);

	fill_vpn_user_name (wrapped_connection);

	settings = nm_connection_to_hash (wrapped_connection);
	utils_clear_filled_connection_certs (wrapped_connection);

	nm_exported_connection_signal_updated (NM_EXPORTED_CONNECTION (self), settings);
	g_hash_table_destroy (settings);
	return TRUE;

invalid:
	g_clear_error (&error);
	nm_exported_connection_signal_removed (NM_EXPORTED_CONNECTION (self));
	return FALSE;
}