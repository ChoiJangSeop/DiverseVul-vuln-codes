is_link_trusted (NautilusFile *file,
		 gboolean is_launcher)
{
	gboolean res;
	
	if (!is_launcher) {
		return TRUE;
	}
	
	if (nautilus_file_can_execute (file)) {
		return TRUE;
	}

	res = FALSE;
	
	if (nautilus_file_is_local (file)) {
		const char * const * data_dirs; 
		char *uri, *path;
		int i;
			
		data_dirs = g_get_system_data_dirs ();
		
		path = NULL;
		uri = nautilus_file_get_uri (file);
		if (uri) {
			path = g_filename_from_uri (uri, NULL, NULL);
			g_free (uri);
		}

		for (i = 0; path != NULL && data_dirs[i] != NULL; i++) {
			if (g_str_has_prefix (path, data_dirs[i])) {
				res = TRUE;
				break;
			}
			
		}
		g_free (path);
	}
	
	
	return res;
}