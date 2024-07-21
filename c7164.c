ebb_ews_connect_sync (EBookMetaBackend *meta_backend,
		      const ENamedParameters *credentials,
		      ESourceAuthenticationResult *out_auth_result,
		      gchar **out_certificate_pem,
		      GTlsCertificateFlags *out_certificate_errors,
		      GCancellable *cancellable,
		      GError **error)
{
	EBookBackendEws *bbews;
	EBookCache *book_cache;
	CamelEwsSettings *ews_settings;
	gchar *hosturl;
	gboolean success = FALSE;

	g_return_val_if_fail (E_IS_BOOK_BACKEND_EWS (meta_backend), FALSE);
	g_return_val_if_fail (out_auth_result != NULL, FALSE);

	bbews = E_BOOK_BACKEND_EWS (meta_backend);

	g_rec_mutex_lock (&bbews->priv->cnc_lock);

	if (bbews->priv->cnc) {
		g_rec_mutex_unlock (&bbews->priv->cnc_lock);

		*out_auth_result = E_SOURCE_AUTHENTICATION_ACCEPTED;

		return TRUE;
	}

	book_cache = e_book_meta_backend_ref_cache (E_BOOK_META_BACKEND (bbews));
	if (book_cache) {
		ECache *cache = E_CACHE (book_cache);
		gint data_version;

		data_version = e_cache_get_key_int (cache, EBB_EWS_DATA_VERSION_KEY, NULL);

		if (data_version != EBB_EWS_DATA_VERSION) {
			MigrateData md;

			e_cache_set_key_int (cache, EBB_EWS_DATA_VERSION_KEY, EBB_EWS_DATA_VERSION, NULL);

			md.data_version = data_version;
			md.is_gal = ebb_ews_check_is_gal (bbews);

			if (e_cache_foreach_update (cache, E_CACHE_INCLUDE_DELETED, NULL, ebb_ews_migrate_data_cb, &md, cancellable, NULL))
				e_cache_sqlite_exec (cache, "vacuum;", cancellable, NULL);
		}

		g_clear_object (&book_cache);
	}

	ews_settings = ebb_ews_get_collection_settings (bbews);
	hosturl = camel_ews_settings_dup_hosturl (ews_settings);

	bbews->priv->cnc = e_ews_connection_new_for_backend (E_BACKEND (bbews), e_book_backend_get_registry (E_BOOK_BACKEND (bbews)), hosturl, ews_settings);

	e_binding_bind_property (
		bbews, "proxy-resolver",
		bbews->priv->cnc, "proxy-resolver",
		G_BINDING_SYNC_CREATE);

	*out_auth_result = e_ews_connection_try_credentials_sync (bbews->priv->cnc, credentials, cancellable, error);

	if (*out_auth_result == E_SOURCE_AUTHENTICATION_ACCEPTED) {
		ESource *source = e_backend_get_source (E_BACKEND (bbews));
		ESourceEwsFolder *ews_folder;

		ews_folder = e_source_get_extension (source, E_SOURCE_EXTENSION_EWS_FOLDER);

		g_free (bbews->priv->folder_id);
		bbews->priv->folder_id = e_source_ews_folder_dup_id (ews_folder);
		bbews->priv->is_gal = ebb_ews_check_is_gal (bbews);

		g_signal_connect_swapped (bbews->priv->cnc, "server-notification",
			G_CALLBACK (ebb_ews_server_notification_cb), bbews);

		if (!bbews->priv->is_gal &&
		    camel_ews_settings_get_listen_notifications (ews_settings) &&
		    e_ews_connection_satisfies_server_version (bbews->priv->cnc, E_EWS_EXCHANGE_2010_SP1)) {
			GSList *folders = NULL;

			folders = g_slist_prepend (folders, bbews->priv->folder_id);

			e_ews_connection_enable_notifications_sync (bbews->priv->cnc,
				folders, &bbews->priv->subscription_key);

			g_slist_free (folders);
		}

		e_book_backend_set_writable (E_BOOK_BACKEND (bbews), !bbews->priv->is_gal);
		success = TRUE;
	} else {
		ebb_ews_convert_error_to_edb_error (error);
		g_clear_object (&bbews->priv->cnc);
	}

	g_rec_mutex_unlock (&bbews->priv->cnc_lock);

	g_free (hosturl);

	return success;
}