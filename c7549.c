_server_connect_start_portal_cb (GObject      *source_object,
                                 GAsyncResult *res,
                                 gpointer      user_data)
{
    GVariant *result;
    GError *error = NULL;

    result = g_dbus_connection_call_finish (G_DBUS_CONNECTION (source_object),
                                            res,
                                            &error);
    if (result != NULL) {
        g_variant_unref (result);
    } else {
        g_print ("portal is not running: %s\n", error->message);
        g_error_free (error);
    }
}