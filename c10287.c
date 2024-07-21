_symlink_is_external_to_destination (GFile      *file,
				     const char *symlink,
				     GFile      *destination,
				     GHashTable *external_links)
{
	gboolean  external = FALSE;
	GFile    *parent;
	char    **components;
	int       i;

	if ((file == NULL) || (symlink == NULL))
		return FALSE;

	if (symlink[0] == '/')
		return TRUE;

	parent = g_file_get_parent (file);
	components = g_strsplit (symlink, "/", -1);
	for (i = 0; components[i] != NULL; i++) {
		char  *name = components[i];
		GFile *tmp;

		if ((name[0] == 0) || ((name[0] == '.') && (name[1] == 0)))
			continue;

		if ((name[0] == '.') && (name[1] == '.') && (name[2] == 0)) {
			if (g_file_equal (parent, destination)) {
				external = TRUE;
				break;
			}
			else {
				tmp = g_file_get_parent (parent);
				g_object_unref (parent);
				parent = tmp;
			}
		}
		else {
			tmp = g_file_get_child (parent, components[i]);
			g_object_unref (parent);
			parent = tmp;
		}

		if (_g_file_is_external_link (parent, destination, external_links)) {
			external = TRUE;
			break;
		}
	}

	g_strfreev (components);
	g_object_unref (parent);

	return external;
}