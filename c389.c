link_info_done (NautilusDirectory *directory,
		NautilusFile *file,
		const char *uri,
		const char *name, 
		const char *icon,
		gboolean is_launcher,
		gboolean is_foreign)
{
	file->details->link_info_is_up_to_date = TRUE;

	nautilus_file_set_display_name (file, name, name, TRUE);
	
	file->details->got_link_info = TRUE;
	g_free (file->details->custom_icon);
	if (uri) {
		if (file->details->activation_location) {
			g_object_unref (file->details->activation_location);
			file->details->activation_location = NULL;
		}
		file->details->got_custom_activation_location = TRUE;
		file->details->activation_location = g_file_new_for_uri (uri);
	}
	file->details->custom_icon = g_strdup (icon);
	file->details->is_launcher = is_launcher;
	file->details->is_foreign_link = is_foreign;
	
	nautilus_directory_async_state_changed (directory);
}