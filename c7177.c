ews_authenticate_sync (CamelService *service,
                       const gchar *mechanism,
                       GCancellable *cancellable,
                       GError **error)
{
	CamelAuthenticationResult result;
	CamelEwsStore *ews_store;
	CamelSettings *settings;
	CamelEwsSettings *ews_settings;
	EEwsConnection *connection;
	ESource *source;
	GSList *folders_created = NULL;
	GSList *folders_updated = NULL;
	GSList *folders_deleted = NULL;
	GSList *folder_ids = NULL;
	GSList *created_folder_ids = NULL;
	gboolean includes_last_folder = FALSE;
	gboolean initial_setup = FALSE;
	const gchar *password;
	gchar *hosturl;
	gchar *old_sync_state = NULL, *new_sync_state = NULL;
	GError *local_error = NULL;

	ews_store = CAMEL_EWS_STORE (service);

	password = camel_service_get_password (service);

	settings = camel_service_ref_settings (service);

	ews_settings = CAMEL_EWS_SETTINGS (settings);
	hosturl = camel_ews_settings_dup_hosturl (ews_settings);
	source = camel_ews_utils_ref_corresponding_source (service, cancellable);

	connection = e_ews_connection_new (source, hosturl, ews_settings);
	e_ews_connection_set_password (connection, password);

	g_clear_object (&source);
	g_free (hosturl);

	g_object_unref (settings);

	e_binding_bind_property (
		service, "proxy-resolver",
		connection, "proxy-resolver",
		G_BINDING_SYNC_CREATE);

	/* XXX We need to run some operation that requires authentication
	 *     but does not change any server-side state, so we can check
	 *     the error status and determine if our password is valid.
	 *     David suggested e_ews_connection_sync_folder_hierarchy(),
	 *     since we have to do that eventually anyway. */

	/*use old sync_state from summary*/
	old_sync_state = camel_ews_store_summary_get_string_val (ews_store->summary, "sync_state", NULL);
	if (!old_sync_state) {
		initial_setup = TRUE;
	} else {
		gchar *folder_id;

		folder_id = camel_ews_store_summary_get_folder_id_from_folder_type (ews_store->summary, CAMEL_FOLDER_TYPE_INBOX);
		if (!folder_id || !*folder_id)
			initial_setup = TRUE;

		g_free (folder_id);

		if (!initial_setup) {
			folder_id = camel_ews_store_summary_get_folder_id_from_folder_type (ews_store->summary, CAMEL_FOLDER_TYPE_DRAFTS);
			if (!folder_id || !*folder_id)
				initial_setup = TRUE;

			g_free (folder_id);
		}
	}

	e_ews_connection_sync_folder_hierarchy_sync (connection, EWS_PRIORITY_MEDIUM, old_sync_state,
		&new_sync_state, &includes_last_folder, &folders_created, &folders_updated, &folders_deleted,
		cancellable, &local_error);

	g_free (old_sync_state);
	old_sync_state = NULL;

	if (g_error_matches (local_error, EWS_CONNECTION_ERROR, EWS_CONNECTION_ERROR_UNAVAILABLE)) {
		local_error->domain = CAMEL_SERVICE_ERROR;
		local_error->code = CAMEL_SERVICE_ERROR_UNAVAILABLE;
	}

	if (!initial_setup && g_error_matches (local_error, EWS_CONNECTION_ERROR, EWS_CONNECTION_ERROR_INVALIDSYNCSTATEDATA)) {
		g_clear_error (&local_error);
		ews_store_forget_all_folders (ews_store);
		camel_ews_store_summary_store_string_val (ews_store->summary, "sync_state", "");
		camel_ews_store_summary_clear (ews_store->summary);

		initial_setup = TRUE;

		e_ews_connection_sync_folder_hierarchy_sync (connection, EWS_PRIORITY_MEDIUM, NULL,
			&new_sync_state, &includes_last_folder, &folders_created, &folders_updated, &folders_deleted,
			cancellable, &local_error);
	}

	if (local_error == NULL) {
		GSList *foreign_fids, *ff;

		g_mutex_lock (&ews_store->priv->connection_lock);
		ews_store_unset_connection_locked (ews_store);
		ews_store->priv->connection = g_object_ref (connection);
		g_signal_connect (ews_store->priv->connection, "password-will-expire",
			G_CALLBACK (camel_ews_store_password_will_expire_cb), ews_store);
		g_mutex_unlock (&ews_store->priv->connection_lock);

		/* This consumes all allocated result data. */
		ews_update_folder_hierarchy (
			ews_store, new_sync_state, includes_last_folder,
			folders_created, folders_deleted, folders_updated, &created_folder_ids);

		/* Also update folder structures of foreign folders,
		   those which are subscribed with subfolders */
		foreign_fids = camel_ews_store_summary_get_foreign_folders (ews_store->summary, NULL);
		for (ff = foreign_fids; ff != NULL; ff = ff->next) {
			const gchar *fid = ff->data;

			if (camel_ews_store_summary_get_foreign_subfolders (ews_store->summary, fid, NULL)) {
				camel_ews_store_update_foreign_subfolders (ews_store, fid);
			}
		}

		g_slist_free_full (foreign_fids, g_free);
	} else {
		g_mutex_lock (&ews_store->priv->connection_lock);
		ews_store_unset_connection_locked (ews_store);
		g_mutex_unlock (&ews_store->priv->connection_lock);

		g_free (new_sync_state);

		/* Make sure we're not leaking anything. */
		g_warn_if_fail (folders_created == NULL);
		g_warn_if_fail (folders_updated == NULL);
		g_warn_if_fail (folders_deleted == NULL);
	}

	/*get folders using distinguished id by GetFolder operation and set system flags to folders, only for first time*/
	if (!local_error && initial_setup && connection) {
		ews_initial_setup_with_connection_sync (CAMEL_STORE (ews_store), NULL, connection, cancellable, NULL);
	}

	/* postpone notification of new folders to time when also folder flags are known,
	   thus the view in evolution sows Inbox with an Inbox icon. */
	for (folder_ids = created_folder_ids; folder_ids; folder_ids = folder_ids->next) {
		CamelFolderInfo *fi;

		fi = camel_ews_utils_build_folder_info (ews_store, folder_ids->data);
		camel_store_folder_created (CAMEL_STORE (ews_store), fi);
		camel_subscribable_folder_subscribed (CAMEL_SUBSCRIBABLE (ews_store), fi);
		camel_folder_info_free (fi);
	}

	g_slist_free_full (created_folder_ids, g_free);

	if (local_error == NULL) {
		result = CAMEL_AUTHENTICATION_ACCEPTED;
	} else if (g_error_matches (local_error, EWS_CONNECTION_ERROR, EWS_CONNECTION_ERROR_AUTHENTICATION_FAILED)) {
		g_clear_error (&local_error);
		result = CAMEL_AUTHENTICATION_REJECTED;
	} else {
		g_propagate_error (error, local_error);
		result = CAMEL_AUTHENTICATION_ERROR;
	}

	g_object_unref (connection);

	return result;
}