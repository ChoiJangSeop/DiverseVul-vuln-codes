read_applet_private_values_from_gconf (NMSetting *setting,
                                       ReadFromGConfInfo *info)
{
	if (NM_IS_SETTING_802_1X (setting)) {
		const char *setting_name = nm_setting_get_name (setting);
		gboolean value;

		if (nm_gconf_get_bool_helper (info->client, info->dir,
		                              NMA_CA_CERT_IGNORE_TAG,
		                              setting_name, &value)) {
			g_object_set_data (G_OBJECT (info->connection),
			                   NMA_CA_CERT_IGNORE_TAG,
			                   GUINT_TO_POINTER (value));
		}

		if (nm_gconf_get_bool_helper (info->client, info->dir,
		                              NMA_PHASE2_CA_CERT_IGNORE_TAG,
		                              setting_name, &value)) {
			g_object_set_data (G_OBJECT (info->connection),
			                   NMA_PHASE2_CA_CERT_IGNORE_TAG,
			                   GUINT_TO_POINTER (value));
		}

		/* Binary certificate and key data doesn't get stored in GConf.  Instead,
		 * the path to the certificate gets stored in a special key and the
		 * certificate is read and stuffed into the setting right before
		 * the connection is sent to NM
		 */
		read_one_cert (info, setting_name, NMA_PATH_CA_CERT_TAG);
		read_one_cert (info, setting_name, NMA_PATH_CLIENT_CERT_TAG);
		read_one_cert (info, setting_name, NMA_PATH_PRIVATE_KEY_TAG);
		read_one_cert (info, setting_name, NMA_PATH_PHASE2_CA_CERT_TAG);
		read_one_cert (info, setting_name, NMA_PATH_PHASE2_CLIENT_CERT_TAG);
		read_one_cert (info, setting_name, NMA_PATH_PHASE2_PRIVATE_KEY_TAG);
	}
}