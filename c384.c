nm_gconf_read_connection (GConfClient *client,
                          const char *dir)
{
	ReadFromGConfInfo info;
	GSList *list;
	GError *err = NULL;

	list = gconf_client_all_dirs (client, dir, &err);
	if (err) {
		g_warning ("Error while reading connection: %s", err->message);
		g_error_free (err);
		return NULL;
	}

	if (!list) {
		g_warning ("Invalid connection (empty)");
		return NULL;
	}

	info.connection = nm_connection_new ();
	info.client = client;
	info.dir = dir;
	info.dir_len = strlen (dir);

	g_slist_foreach (list, read_one_setting, &info);
	g_slist_free (list);

	return info.connection;
}