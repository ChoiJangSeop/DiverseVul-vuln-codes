ews_backend_delete_resource_sync (ECollectionBackend *backend,
                                  ESource *source,
                                  GCancellable *cancellable,
                                  GError **error)
{
	EEwsConnection *connection;
	ESourceEwsFolder *extension;
	const gchar *extension_name;
	gboolean success = FALSE;

	connection = e_ews_backend_ref_connection_sync (E_EWS_BACKEND (backend), NULL, cancellable, error);
	if (connection == NULL)
		return FALSE;

	extension_name = E_SOURCE_EXTENSION_EWS_FOLDER;
	if (!e_source_has_extension (source, extension_name)) {
		g_set_error (
			error, G_IO_ERROR,
			G_IO_ERROR_INVALID_ARGUMENT,
			_("Data source “%s” does not represent "
			"an Exchange Web Services folder"),
			e_source_get_display_name (source));
		goto exit;
	}
	extension = e_source_get_extension (source, extension_name);

	if (e_source_ews_folder_get_foreign (extension) ||
	    e_source_ews_folder_get_public (extension)) {
		/* do not delete foreign or public folders,
		 * just remove them from local store */
		success = TRUE;
	} else {
		gchar *folder_id;

		folder_id = e_source_ews_folder_dup_id (extension);

		success = e_ews_connection_delete_folder_sync (
			connection, EWS_PRIORITY_MEDIUM, folder_id,
			FALSE, "HardDelete", cancellable, error);

		g_free (folder_id);
	}

	if (success)
		success = e_source_remove_sync (source, cancellable, error);

exit:
	g_object_unref (connection);

	return success;
}