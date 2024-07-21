update_connection (NMConnectionList *list,
                   NMConnectionEditor *editor,
                   NMExportedConnection *original,
                   NMConnection *modified,
                   ConnectionUpdatedFn callback,
                   gpointer user_data)
{
	NMConnectionScope original_scope;
	ConnectionUpdateInfo *info;

	info = g_slice_new0 (ConnectionUpdateInfo);
	info->list = list;
	info->editor = editor;
	info->original = g_object_ref (original);
	info->modified = g_object_ref (modified);
	info->callback = callback;
	info->user_data = user_data;

	original_scope = nm_connection_get_scope (nm_exported_connection_get_connection (original));
	if (nm_connection_get_scope (modified) == original_scope) {
		/* The easy part: Connection is updated */
		GHashTable *new_settings;
		GError *error = NULL;
		gboolean success;
		gboolean pending_auth = FALSE;
		GtkWindow *parent;

		utils_fill_connection_certs (modified);
		new_settings = nm_connection_to_hash (modified);

		/* Hack; make sure that gconf private values are copied */
		nm_gconf_copy_private_connection_values (nm_exported_connection_get_connection (original),
		                                         modified);

		success = nm_exported_connection_update (original, new_settings, &error);
		g_hash_table_destroy (new_settings);
		utils_clear_filled_connection_certs (modified);

		parent = nm_connection_editor_get_window (editor);
		if (!success) {
			if (pk_helper_is_permission_denied_error (error)) {
				GError *auth_error = NULL;

				pending_auth = pk_helper_obtain_auth (error, parent, update_connection_cb, info, &auth_error);
				if (auth_error) {
					error_dialog (parent,
					              _("Could not update connection"),
					              "%s", auth_error->message);
					g_error_free (auth_error);
				}
			} else {
				error_dialog (parent,
				              _("Could not update connection"),
				              "%s", error->message);
			}

			g_error_free (error);
		} else {
			/* Save user-connection vpn secrets */
			if (editor && (original_scope == NM_CONNECTION_SCOPE_USER))
				nm_connection_editor_save_vpn_secrets (editor);
		}

		if (!pending_auth)
			connection_update_done (info, success);
	} else {
		/* The hard part: Connection scope changed:
		   Add the exported connection,
		   if it succeeds, remove the old one. */
		add_connection (list, editor, modified, connection_update_add_done, info);
	}
}