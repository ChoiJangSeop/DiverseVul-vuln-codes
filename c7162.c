e_ews_backend_sync_folders_sync (EEwsBackend *backend,
                                 GCancellable *cancellable,
                                 GError **error)
{
	EEwsConnection *connection;
	GSList *folders_created = NULL;
	GSList *folders_updated = NULL;
	GSList *folders_deleted = NULL;
	gboolean includes_last_folder = FALSE;
	gchar *old_sync_state, *new_sync_state = NULL;
	gboolean success;
	GError *local_error = NULL;

	g_return_val_if_fail (E_IS_EWS_BACKEND (backend), FALSE);

	if (!e_backend_get_online (E_BACKEND (backend))) {
		SyncFoldersClosure *closure;

		/* This takes ownership of the folder lists. */
		closure = g_slice_new0 (SyncFoldersClosure);
		closure->backend = g_object_ref (backend);

		/* Process the results from an idle callback. */
		g_idle_add_full (
			G_PRIORITY_DEFAULT_IDLE,
			ews_backend_sync_folders_idle_cb, closure,
			(GDestroyNotify) sync_folders_closure_free);

		return TRUE;
	}

	connection = e_ews_backend_ref_connection_sync (backend, NULL, cancellable, error);

	if (connection == NULL) {
		backend->priv->need_update_folders = TRUE;
		return FALSE;
	}

	backend->priv->need_update_folders = FALSE;

	g_mutex_lock (&backend->priv->sync_state_lock);
	old_sync_state = g_strdup (backend->priv->sync_state);
	g_mutex_unlock (&backend->priv->sync_state_lock);

	success = e_ews_connection_sync_folder_hierarchy_sync (connection, EWS_PRIORITY_MEDIUM, old_sync_state,
		&new_sync_state, &includes_last_folder, &folders_created, &folders_updated, &folders_deleted,
		cancellable, &local_error);

	if (old_sync_state && g_error_matches (local_error, EWS_CONNECTION_ERROR, EWS_CONNECTION_ERROR_INVALIDSYNCSTATEDATA)) {
		g_clear_error (&local_error);

		g_mutex_lock (&backend->priv->sync_state_lock);
		g_free (backend->priv->sync_state);
		backend->priv->sync_state = NULL;
		g_mutex_unlock (&backend->priv->sync_state_lock);

		ews_backend_forget_all_sources (backend);

		success = e_ews_connection_sync_folder_hierarchy_sync (connection, EWS_PRIORITY_MEDIUM, NULL,
			&new_sync_state, &includes_last_folder, &folders_created, &folders_updated, &folders_deleted,
			cancellable, &local_error);
	} else if (local_error) {
		g_propagate_error (error, local_error);
		local_error = NULL;
	}

	g_free (old_sync_state);
	old_sync_state = NULL;

	if (success) {
		SyncFoldersClosure *closure;

		/* This takes ownership of the folder lists. */
		closure = g_slice_new0 (SyncFoldersClosure);
		closure->backend = g_object_ref (backend);
		closure->folders_created = folders_created;
		closure->folders_deleted = folders_deleted;
		closure->folders_updated = folders_updated;

		/* Process the results from an idle callback. */
		g_idle_add_full (
			G_PRIORITY_DEFAULT_IDLE,
			ews_backend_sync_folders_idle_cb, closure,
			(GDestroyNotify) sync_folders_closure_free);

		g_mutex_lock (&backend->priv->sync_state_lock);
		g_free (backend->priv->sync_state);
		backend->priv->sync_state = g_strdup (new_sync_state);
		g_mutex_unlock (&backend->priv->sync_state_lock);

	} else {
		/* Make sure we're not leaking anything. */
		g_warn_if_fail (folders_created == NULL);
		g_warn_if_fail (folders_updated == NULL);
		g_warn_if_fail (folders_deleted == NULL);

		backend->priv->need_update_folders = TRUE;
	}

	g_free (new_sync_state);

	g_object_unref (connection);

	return success;
}