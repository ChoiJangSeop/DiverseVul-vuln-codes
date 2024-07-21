_g_file_is_external_link (GFile      *file,
			  GFile      *destination,
			  GHashTable *external_links)
{
	GFileInfo *info;
	gboolean   external;

	if (g_hash_table_lookup (external_links, file) != NULL)
		return TRUE;

	info = g_file_query_info (file,
				  G_FILE_ATTRIBUTE_STANDARD_IS_SYMLINK "," G_FILE_ATTRIBUTE_STANDARD_SYMLINK_TARGET,
				  G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
				  NULL,
				  NULL);

	if (info == NULL)
		return FALSE;

	external = FALSE;

	if (g_file_info_get_is_symlink (info)) {
		if (_symlink_is_external_to_destination (file,
							 g_file_info_get_symlink_target (info),
							 destination,
							 external_links))
		{
			g_hash_table_insert (external_links, g_object_ref (file), GINT_TO_POINTER (1));
			external = TRUE;
		}
	}

	g_object_unref (info);

	return external;
}