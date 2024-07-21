add_connection (NMConnectionList *self,
                NMConnectionEditor *editor,
                NMConnection *connection,
                ConnectionAddedFn callback,
                gpointer user_data)
{
	NMExportedConnection *exported = NULL;
	NMConnectionScope scope;
	gboolean success = FALSE;

	scope = nm_connection_get_scope (connection);
	if (scope == NM_CONNECTION_SCOPE_SYSTEM) {
		GError *error = NULL;

		utils_fill_connection_certs (connection);
		success = nm_dbus_settings_system_add_connection (self->system_settings, connection, &error);
		utils_clear_filled_connection_certs (connection);

		if (!success) {
			gboolean pending_auth = FALSE;
			GtkWindow *parent;

			parent = nm_connection_editor_get_window (editor);
			if (pk_helper_is_permission_denied_error (error)) {
				ConnectionAddInfo *info;
				GError *auth_error = NULL;

				info = g_slice_new (ConnectionAddInfo);
				info->list = self;
				info->editor = editor;
				info->connection = g_object_ref (connection);
				info->callback = callback;
				info->user_data = user_data;

				pending_auth = pk_helper_obtain_auth (error, parent, add_connection_cb, info, &auth_error);
				if (auth_error) {
					error_dialog (parent,
					              _("Could not add connection"),
					              "%s", auth_error->message);
					g_error_free (auth_error);
				}

				if (!pending_auth) {
					g_object_unref (info->connection);
					g_slice_free (ConnectionAddInfo, info);
				}
			} else {
				error_dialog (parent,
				              _("Could not add connection"),
				              "%s", error->message);
			}

			g_error_free (error);

			if (pending_auth)
				return;
		}
	} else if (scope == NM_CONNECTION_SCOPE_USER) {
		exported = (NMExportedConnection *) nma_gconf_settings_add_connection (self->gconf_settings, connection);
		success = exported != NULL;
		if (success && editor)
			nm_connection_editor_save_vpn_secrets (editor);
	} else
		g_warning ("%s: unhandled connection scope %d!", __func__, scope);

	if (callback)
		callback (exported, success, user_data);

	if (exported)
		g_object_unref (exported);
}