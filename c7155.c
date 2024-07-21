ecb_ews_connect_sync (ECalMetaBackend *meta_backend,
		      const ENamedParameters *credentials,
		      ESourceAuthenticationResult *out_auth_result,
		      gchar **out_certificate_pem,
		      GTlsCertificateFlags *out_certificate_errors,
		      GCancellable *cancellable,
		      GError **error)
{
	ECalBackendEws *cbews;
	CamelEwsSettings *ews_settings;
	gchar *hosturl;
	gboolean success = FALSE;

	g_return_val_if_fail (E_IS_CAL_BACKEND_EWS (meta_backend), FALSE);
	g_return_val_if_fail (out_auth_result != NULL, FALSE);

	cbews = E_CAL_BACKEND_EWS (meta_backend);

	g_rec_mutex_lock (&cbews->priv->cnc_lock);

	if (cbews->priv->cnc) {
		g_rec_mutex_unlock (&cbews->priv->cnc_lock);

		*out_auth_result = E_SOURCE_AUTHENTICATION_ACCEPTED;

		return TRUE;
	}

	ews_settings = ecb_ews_get_collection_settings (cbews);
	hosturl = camel_ews_settings_dup_hosturl (ews_settings);

	cbews->priv->cnc = e_ews_connection_new_for_backend (E_BACKEND (cbews), e_cal_backend_get_registry (E_CAL_BACKEND (cbews)), hosturl, ews_settings);

	e_binding_bind_property (
		cbews, "proxy-resolver",
		cbews->priv->cnc, "proxy-resolver",
		G_BINDING_SYNC_CREATE);

	*out_auth_result = e_ews_connection_try_credentials_sync (cbews->priv->cnc, credentials, cancellable, error);

	if (*out_auth_result == E_SOURCE_AUTHENTICATION_ACCEPTED) {
		ESource *source = e_backend_get_source (E_BACKEND (cbews));
		ESourceEwsFolder *ews_folder;

		ews_folder = e_source_get_extension (source, E_SOURCE_EXTENSION_EWS_FOLDER);

		g_free (cbews->priv->folder_id);
		cbews->priv->folder_id = e_source_ews_folder_dup_id (ews_folder);
		cbews->priv->is_freebusy_calendar = cbews->priv->folder_id && g_str_has_prefix (cbews->priv->folder_id, "freebusy-calendar::");

		g_signal_connect_swapped (cbews->priv->cnc, "server-notification",
			G_CALLBACK (ecb_ews_server_notification_cb), cbews);

		if (!cbews->priv->is_freebusy_calendar &&
		    camel_ews_settings_get_listen_notifications (ews_settings) &&
		    e_ews_connection_satisfies_server_version (cbews->priv->cnc, E_EWS_EXCHANGE_2010_SP1)) {
			GSList *folders = NULL;

			folders = g_slist_prepend (folders, cbews->priv->folder_id);

			e_ews_connection_enable_notifications_sync (cbews->priv->cnc,
				folders, &cbews->priv->subscription_key);

			g_slist_free (folders);
		}

		e_cal_backend_set_writable (E_CAL_BACKEND (cbews), !cbews->priv->is_freebusy_calendar);
		success = TRUE;
	} else {
		ecb_ews_convert_error_to_edc_error (error);
		g_clear_object (&cbews->priv->cnc);
	}

	g_rec_mutex_unlock (&cbews->priv->cnc_lock);

	g_free (hosturl);

	return success;
}