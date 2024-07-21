utils_fill_connection_certs (NMConnection *connection)
{
	NMSetting8021x *s_8021x;
	const char *filename;
	GError *error = NULL;
	gboolean need_client_cert = TRUE;

	g_return_if_fail (connection != NULL);

	s_8021x = NM_SETTING_802_1X (nm_connection_get_setting (connection, NM_TYPE_SETTING_802_1X));
	if (!s_8021x)
		return;

	filename = g_object_get_data (G_OBJECT (connection), NMA_PATH_CA_CERT_TAG);
	if (filename) {
		if (!nm_setting_802_1x_set_ca_cert_from_file (s_8021x, filename, NULL, &error))
			g_warning ("%s: couldn't read CA certificate: %d %s", __func__, error->code, error->message);
		g_clear_error (&error);
	}

	/* If the private key is PKCS#12, don't set the client cert */
	need_client_cert = fill_one_private_key (connection,
	                                         NMA_PATH_PRIVATE_KEY_TAG,
	                                         NM_SETTING_802_1X_PRIVATE_KEY,
	                                         NM_SETTING_802_1X_CLIENT_CERT);
	if (need_client_cert) {
		filename = g_object_get_data (G_OBJECT (connection), NMA_PATH_CLIENT_CERT_TAG);
		if (filename) {
			if (!nm_setting_802_1x_set_client_cert_from_file (s_8021x, filename, NULL, &error))
				g_warning ("%s: couldn't read client certificate: %d %s", __func__, error->code, error->message);
			g_clear_error (&error);
		}
	}

	filename = g_object_get_data (G_OBJECT (connection), NMA_PATH_PHASE2_CA_CERT_TAG);
	if (filename) {
		if (!nm_setting_802_1x_set_phase2_ca_cert_from_file (s_8021x, filename, NULL, &error))
			g_warning ("%s: couldn't read phase2 CA certificate: %d %s", __func__, error->code, error->message);
		g_clear_error (&error);
	}

	/* If the private key is PKCS#12, don't set the client cert */
	need_client_cert = fill_one_private_key (connection,
	                                         NMA_PATH_PHASE2_PRIVATE_KEY_TAG,
	                                         NM_SETTING_802_1X_PHASE2_PRIVATE_KEY,
	                                         NM_SETTING_802_1X_PHASE2_CLIENT_CERT);
	if (need_client_cert) {
		filename = g_object_get_data (G_OBJECT (connection), NMA_PATH_PHASE2_CLIENT_CERT_TAG);
		if (filename) {
			if (!nm_setting_802_1x_set_phase2_client_cert_from_file (s_8021x, filename, NULL, &error))
				g_warning ("%s: couldn't read phase2 client certificate: %d %s", __func__, error->code, error->message);
			g_clear_error (&error);
		}
	}
}