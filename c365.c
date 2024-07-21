nm_connection_list_new (GType def_type)
{
	NMConnectionList *list;
	DBusGConnection *dbus_connection;
	GError *error = NULL;

	list = g_object_new (NM_TYPE_CONNECTION_LIST, NULL);
	if (!list)
		return NULL;

	/* load GUI */
	list->gui = glade_xml_new (GLADEDIR "/nm-connection-editor.glade", "NMConnectionList", NULL);
	if (!list->gui) {
		g_warning ("Could not load Glade file for connection list");
		goto error;
	}

	gtk_window_set_default_icon_name ("preferences-system-network");

	list->icon_theme = gtk_icon_theme_get_for_screen (gdk_screen_get_default ());

	/* Load icons */
	ICON_LOAD(list->wired_icon, "nm-device-wired");
	ICON_LOAD(list->wireless_icon, "nm-device-wireless");
	ICON_LOAD(list->wwan_icon, "nm-device-wwan");
	ICON_LOAD(list->vpn_icon, "nm-vpn-standalone-lock");
	ICON_LOAD(list->unknown_icon, "nm-no-connection");

	list->client = gconf_client_get_default ();
	if (!list->client)
		goto error;

	dbus_connection = dbus_g_bus_get (DBUS_BUS_SYSTEM, &error);
	if (error) {
		g_warning ("Could not connect to the system bus: %s", error->message);
		g_error_free (error);
		goto error;
	}

	list->system_settings = nm_dbus_settings_system_new (dbus_connection);
	dbus_g_connection_unref (dbus_connection);
	g_signal_connect (list->system_settings, "new-connection",
				   G_CALLBACK (connection_added),
				   list);

	list->gconf_settings = nma_gconf_settings_new ();
	g_signal_connect (list->gconf_settings, "new-connection",
				   G_CALLBACK (connection_added),
				   list);

	add_connection_tabs (list, def_type);

	list->editors = g_hash_table_new_full (g_direct_hash, g_direct_equal, g_object_unref, g_object_unref);

	list->dialog = glade_xml_get_widget (list->gui, "NMConnectionList");
	if (!list->dialog)
		goto error;
	g_signal_connect (G_OBJECT (list->dialog), "response", G_CALLBACK (dialog_response_cb), list);

	if (!vpn_get_plugins (&error)) {
		g_warning ("%s: failed to load VPN plugins: %s", __func__, error->message);
		g_error_free (error);
	}

	return list;

error:
	g_object_unref (list);
	return NULL;
}