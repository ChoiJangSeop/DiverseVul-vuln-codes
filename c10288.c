_g_path_is_external_to_destination (const char *relative_path,
				    GFile      *destination,
				    GHashTable *external_links)
{
	gboolean  external = FALSE;
	GFile    *parent;
	char    **components;
	int       i;

	if (relative_path == NULL)
		return FALSE;

	if (destination == NULL)
		return TRUE;

	parent = g_object_ref (destination);
	components = g_strsplit (relative_path, "/", -1);
	for (i = 0; (components[i] != NULL) && (components[i + 1] != NULL); i++) {
		GFile *tmp;

		if (components[i][0] == 0)
			continue;

		tmp = g_file_get_child (parent, components[i]);
		g_object_unref (parent);
		parent = tmp;

		if (_g_file_is_external_link (parent, destination, external_links)) {
			external = TRUE;
			break;
		}
	}

	g_strfreev (components);
	g_object_unref (parent);

	return external;
}