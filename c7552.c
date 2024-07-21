bus_acquired_handler (GDBusConnection *connection,
                      const gchar     *name,
                      gpointer         user_data)
{
    g_dbus_connection_call (connection,
                            IBUS_SERVICE_PORTAL,
                            IBUS_PATH_IBUS,
                            "org.freedesktop.DBus.Peer",
                            "Ping",
                            g_variant_new ("()"),
                            G_VARIANT_TYPE ("()"),
                            G_DBUS_CALL_FLAGS_NONE,
                            -1,
                            NULL /* cancellable */,
                            (GAsyncReadyCallback)
                                    _server_connect_start_portal_cb,
                            NULL);
}