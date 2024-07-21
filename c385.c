edit_done_cb (NMConnectionEditor *editor, gint response, GError *error, gpointer user_data)
{
	EditConnectionInfo *info = (EditConnectionInfo *) user_data;
	const char *message = _("An unknown error ocurred.");

	g_hash_table_remove (info->list->editors, info->original_connection);

	if (response == GTK_RESPONSE_NONE) {
		if (error && error->message)
			message = error->message;
		error_dialog (GTK_WINDOW (editor->window), _("Error initializing editor"), "%s", message);
	} else if (response == GTK_RESPONSE_OK) {
		NMConnection *connection;
		GError *edit_error = NULL;
		gboolean success;

		connection = nm_connection_editor_get_connection (editor);

		utils_fill_connection_certs (connection);
		success = nm_connection_verify (connection, &edit_error);
		utils_clear_filled_connection_certs (connection);

		if (success) {
			update_connection (info->list, editor, info->original_connection,
			                   connection, connection_updated_cb, info);
		} else {
			g_warning ("%s: invalid connection after update: bug in the "
			           "'%s' / '%s' invalid: %d",
			           __func__,
			           g_type_name (nm_connection_lookup_setting_type_by_quark (edit_error->domain)),
			           edit_error->message, edit_error->code);
			g_error_free (edit_error);
			connection_updated_cb (info->list, FALSE, user_data);
		}
	}
}