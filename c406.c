nautilus_link_get_link_info_given_file_contents (const char  *file_contents,
						 int          link_file_size,
						 const char  *file_uri,
						 char       **uri,
						 char       **name,
						 char       **icon,
						 gboolean    *is_launcher,
						 gboolean    *is_foreign)
{
	GKeyFile *key_file;
	char *type;
	char **only_show_in;
	char **not_show_in;

	if (!is_link_data (file_contents, link_file_size)) {
		return;
	}

	key_file = g_key_file_new ();
	if (!g_key_file_load_from_data (key_file,
					file_contents,
					link_file_size,
					G_KEY_FILE_NONE,
					NULL)) {
		g_key_file_free (key_file);
		return; 
	}

	*uri = nautilus_link_get_link_uri_from_desktop (key_file, file_uri);
	*name = nautilus_link_get_link_name_from_desktop (key_file);
	*icon = nautilus_link_get_link_icon_from_desktop (key_file);

	*is_launcher = FALSE;
	type = g_key_file_get_string (key_file, MAIN_GROUP, "Type", NULL);
	if (g_strcmp0 (type, "Application") == 0 &&
	    g_key_file_has_key (key_file, MAIN_GROUP, "Exec", NULL)) {
		*is_launcher = TRUE;
	}
	g_free (type);

	*is_foreign = FALSE;
	only_show_in = g_key_file_get_string_list (key_file, MAIN_GROUP,
						   "OnlyShowIn", NULL, NULL);
	if (only_show_in && !string_array_contains (only_show_in, "GNOME")) {
		*is_foreign = TRUE;
	}
	g_strfreev (only_show_in);

	not_show_in = g_key_file_get_string_list (key_file, MAIN_GROUP,
						  "NotShowIn", NULL, NULL);
	if (not_show_in && string_array_contains (not_show_in, "GNOME")) {
		*is_foreign = TRUE;
	}
	g_strfreev (not_show_in);

	g_key_file_free (key_file);
}