mark_desktop_files_trusted (void)
{
	char *user_dir, *do_once_file;
	GFile *f, *c;
	GFileEnumerator *e;
	GFileInfo *info;
	const char *name;
	int fd;
	
	user_dir = nautilus_get_user_directory ();
	do_once_file = g_build_filename (user_dir, "converted-launchers", NULL);
	g_free (user_dir);

	if (g_file_test (do_once_file, G_FILE_TEST_EXISTS)) {
		goto out;
	}

	f = nautilus_get_desktop_location ();
	e = g_file_enumerate_children (f,
				       G_FILE_ATTRIBUTE_STANDARD_TYPE ","
				       G_FILE_ATTRIBUTE_STANDARD_NAME ","
				       G_FILE_ATTRIBUTE_ACCESS_CAN_EXECUTE
				       ,
				       G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
				       NULL, NULL);
	if (e == NULL) {
		goto out2;
	}
	
	while ((info = g_file_enumerator_next_file (e, NULL, NULL)) != NULL) {
		name = g_file_info_get_name (info);
		
		if (g_str_has_suffix (name, ".desktop") &&
		    !g_file_info_get_attribute_boolean (info, G_FILE_ATTRIBUTE_ACCESS_CAN_EXECUTE)) {
			c = g_file_get_child (f, name);
			nautilus_file_mark_desktop_file_trusted (c,
								 NULL, FALSE,
								 NULL, NULL);
			g_object_unref (c);
		}
		g_object_unref (info);
	}
	
	g_object_unref (e);
 out2:
	fd = g_creat (do_once_file, 0666);
	close (fd);
	
	g_object_unref (f);
 out:	
	g_free (do_once_file);
}