untrusted_launcher_response_callback (GtkDialog *dialog,
				      int response_id,
				      ActivateParametersDesktop *parameters)
{
	GdkScreen *screen;
	char *uri;
	gboolean free_params;
	
	free_params = TRUE;
	switch (response_id) {
	case RESPONSE_RUN:
		screen = gtk_widget_get_screen (GTK_WIDGET (parameters->parent_window));
		uri = nautilus_file_get_uri (parameters->file);
		nautilus_debug_log (FALSE, NAUTILUS_DEBUG_LOG_DOMAIN_USER,
				    "directory view activate_callback launch_desktop_file window=%p: %s",
				    parameters->parent_window, uri);
		nautilus_launch_desktop_file (screen, uri, NULL,
					      parameters->parent_window);
		g_free (uri);
		break;
	case RESPONSE_MARK_TRUSTED:
		nautilus_file_set_permissions (parameters->file, 
					       nautilus_file_get_permissions (parameters->file) | S_IXGRP | S_IXUSR | S_IXOTH,
					       mark_trusted_callback,
					       parameters);
		free_params = FALSE;
		break;
	default:
		/* Just destroy dialog */
		break;
	}
	
	gtk_widget_destroy (GTK_WIDGET (dialog));
	if (free_params) {
		activate_parameters_desktop_free (parameters);
	}
}