constructor (GType type,
             guint n_props,
             GObjectConstructParam *construct_props)
{
	NMApplet *applet;
	AppletDBusManager *dbus_mgr;
	GList *server_caps, *iter;

	applet = NM_APPLET (G_OBJECT_CLASS (nma_parent_class)->constructor (type, n_props, construct_props));

	g_set_application_name (_("NetworkManager Applet"));
	gtk_window_set_default_icon_name (GTK_STOCK_NETWORK);

	applet->glade_file = g_build_filename (GLADEDIR, "applet.glade", NULL);
	if (!applet->glade_file || !g_file_test (applet->glade_file, G_FILE_TEST_IS_REGULAR)) {
		GtkWidget *dialog;
		dialog = applet_warning_dialog_show (_("The NetworkManager Applet could not find some required resources (the glade file was not found)."));
		gtk_dialog_run (GTK_DIALOG (dialog));
		goto error;
	}

	applet->info_dialog_xml = glade_xml_new (applet->glade_file, "info_dialog", NULL);
	if (!applet->info_dialog_xml)
		goto error;

	applet->gconf_client = gconf_client_get_default ();
	if (!applet->gconf_client)
		goto error;

	/* Load pixmaps and create applet widgets */
	if (!setup_widgets (applet))
		goto error;
	nma_icons_init (applet);

	if (!notify_is_initted ())
		notify_init ("NetworkManager");

	server_caps = notify_get_server_caps();
	applet->notify_with_actions = FALSE;
	for (iter = server_caps; iter; iter = g_list_next (iter)) {
		if (!strcmp ((const char *) iter->data, NOTIFY_CAPS_ACTIONS_KEY))
			applet->notify_with_actions = TRUE;
	}

	g_list_foreach (server_caps, (GFunc) g_free, NULL);
	g_list_free (server_caps);

	dbus_mgr = applet_dbus_manager_get ();
	if (dbus_mgr == NULL) {
		nm_warning ("Couldn't initialize the D-Bus manager.");
		g_object_unref (applet);
		return NULL;
	}
	g_signal_connect (G_OBJECT (dbus_mgr), "exit-now", G_CALLBACK (exit_cb), applet);

	applet->dbus_settings = (NMDBusSettings *) nm_dbus_settings_system_new (applet_dbus_manager_get_connection (dbus_mgr));

	applet->gconf_settings = nma_gconf_settings_new ();
	g_signal_connect (applet->gconf_settings, "new-secrets-requested",
	                  G_CALLBACK (applet_settings_new_secrets_requested_cb),
	                  applet);

	dbus_g_connection_register_g_object (applet_dbus_manager_get_connection (dbus_mgr),
	                                     NM_DBUS_PATH_SETTINGS,
	                                     G_OBJECT (applet->gconf_settings));

	/* Start our DBus service */
	if (!applet_dbus_manager_start_service (dbus_mgr)) {
		g_object_unref (applet);
		return NULL;
	}

	/* Initialize device classes */
	applet->wired_class = applet_device_wired_get_class (applet);
	g_assert (applet->wired_class);

	applet->wifi_class = applet_device_wifi_get_class (applet);
	g_assert (applet->wifi_class);

	applet->gsm_class = applet_device_gsm_get_class (applet);
	g_assert (applet->gsm_class);

	applet->cdma_class = applet_device_cdma_get_class (applet);
	g_assert (applet->cdma_class);

	foo_client_setup (applet);

	/* timeout to update connection timestamps every 5 minutes */
	applet->update_timestamps_id = g_timeout_add_seconds (300,
			(GSourceFunc) periodic_update_active_connection_timestamps, applet);

	nm_gconf_set_pre_keyring_callback (applet_pre_keyring_callback, applet);

	return G_OBJECT (applet);

error:
	g_object_unref (applet);
	return NULL;
}