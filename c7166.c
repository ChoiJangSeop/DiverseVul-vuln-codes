ews_backend_create_resource_sync (ECollectionBackend *backend,
                                  ESource *source,
                                  GCancellable *cancellable,
                                  GError **error)
{
	EEwsConnection *connection = NULL;
	EwsFolderId *out_folder_id = NULL;
	EEwsFolderType folder_type = E_EWS_FOLDER_TYPE_UNKNOWN;
	const gchar *extension_name;
	const gchar *parent_folder_id = NULL;
	gchar *folder_name;
	gboolean success = FALSE;

	extension_name = E_SOURCE_EXTENSION_EWS_FOLDER;
	if (e_source_has_extension (source, extension_name)) {
		ESourceEwsFolder *extension;

		/* foreign and public folders are just added */
		extension = e_source_get_extension (source, extension_name);
		if (e_source_ews_folder_get_foreign (extension) ||
		    e_source_ews_folder_get_public (extension))
			success = TRUE;
	}

	if (!success) {
		connection = e_ews_backend_ref_connection_sync (E_EWS_BACKEND (backend), NULL, cancellable, error);
		if (connection == NULL)
			return FALSE;

		extension_name = E_SOURCE_EXTENSION_ADDRESS_BOOK;
		if (e_source_has_extension (source, extension_name)) {
			folder_type = E_EWS_FOLDER_TYPE_CONTACTS;
			parent_folder_id = "contacts";
		}

		extension_name = E_SOURCE_EXTENSION_CALENDAR;
		if (e_source_has_extension (source, extension_name)) {
			folder_type = E_EWS_FOLDER_TYPE_CALENDAR;
			parent_folder_id = "calendar";
		}

		extension_name = E_SOURCE_EXTENSION_TASK_LIST;
		if (e_source_has_extension (source, extension_name)) {
			folder_type = E_EWS_FOLDER_TYPE_TASKS;
			parent_folder_id = "tasks";
		}

		/* FIXME No support for memo lists. */

		if (parent_folder_id == NULL) {
			g_set_error (
				error, G_IO_ERROR,
				G_IO_ERROR_INVALID_ARGUMENT,
				_("Could not determine a suitable folder "
				"class for a new folder named “%s”"),
				e_source_get_display_name (source));
			goto exit;
		}

		folder_name = e_source_dup_display_name (source);

		success = e_ews_connection_create_folder_sync (
			connection, EWS_PRIORITY_MEDIUM,
			parent_folder_id, TRUE,
			folder_name, folder_type,
			&out_folder_id, cancellable, error);

		g_free (folder_name);

		/* Sanity check */
		g_warn_if_fail (
			(success && out_folder_id != NULL) ||
			(!success && out_folder_id == NULL));

		if (out_folder_id != NULL) {
			ESourceEwsFolder *extension;
			const gchar *extension_name;

			extension_name = E_SOURCE_EXTENSION_EWS_FOLDER;
			extension = e_source_get_extension (source, extension_name);
			e_source_ews_folder_set_id (
				extension, out_folder_id->id);
			e_source_ews_folder_set_change_key (
				extension, out_folder_id->change_key);

			e_ews_folder_id_free (out_folder_id);
		}
	}

	if (success) {
		ESourceRegistryServer *server;
		ESource *parent_source;
		const gchar *cache_dir;
		const gchar *parent_uid;

		/* Configure the source as a collection member. */
		parent_source = e_backend_get_source (E_BACKEND (backend));
		parent_uid = e_source_get_uid (parent_source);
		e_source_set_parent (source, parent_uid);

		/* Changes should be written back to the cache directory. */
		cache_dir = e_collection_backend_get_cache_dir (backend);
		e_server_side_source_set_write_directory (
			E_SERVER_SIDE_SOURCE (source), cache_dir);

		/* Set permissions for clients. */
		e_server_side_source_set_writable (E_SERVER_SIDE_SOURCE (source), TRUE);
		e_server_side_source_set_remote_deletable (E_SERVER_SIDE_SOURCE (source), TRUE);

		server = e_collection_backend_ref_server (backend);
		e_source_registry_server_add_source (server, source);
		g_object_unref (server);
	}

 exit:
	if (connection)
		g_object_unref (connection);

	return success;
}