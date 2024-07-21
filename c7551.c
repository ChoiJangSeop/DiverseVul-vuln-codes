bus_server_init (void)
{
    GError *error = NULL;

    dbus = bus_dbus_impl_get_default ();
    ibus = bus_ibus_impl_get_default ();
    bus_dbus_impl_register_object (dbus, (IBusService *)ibus);

    /* init server */
    GDBusServerFlags flags = G_DBUS_SERVER_FLAGS_AUTHENTICATION_ALLOW_ANONYMOUS;
    gchar *guid = g_dbus_generate_guid ();
    if (!g_str_has_prefix (g_address, "unix:tmpdir=") &&
        !g_str_has_prefix (g_address, "unix:path=")) {
        g_error ("Your socket address does not have the format unix:tmpdir=$DIR "
                 "or unix:path=$FILE; %s", g_address);
    }
    server =  g_dbus_server_new_sync (
                    g_address, /* the place where the socket file lives, e.g. /tmp, abstract namespace, etc. */
                    flags, guid,
                    NULL /* observer */,
                    NULL /* cancellable */,
                    &error);
    if (server == NULL) {
        g_error ("g_dbus_server_new_sync() is failed with address %s "
                 "and guid %s: %s",
                 g_address, guid, error->message);
    }
    g_free (guid);

    g_signal_connect (server, "new-connection", G_CALLBACK (bus_new_connection_cb), NULL);

    g_dbus_server_start (server);

    address = g_strdup_printf ("%s,guid=%s",
                               g_dbus_server_get_client_address (server),
                               g_dbus_server_get_guid (server));

    /* write address to file */
    ibus_write_address (address);

    /* own a session bus name so that third parties can easily track our life-cycle */
    g_bus_own_name (G_BUS_TYPE_SESSION, IBUS_SERVICE_IBUS,
                    G_BUS_NAME_OWNER_FLAGS_NONE,
                    bus_acquired_handler,
                    NULL, NULL, NULL, NULL);
}