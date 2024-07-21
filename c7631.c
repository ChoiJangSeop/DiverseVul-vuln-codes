handle_get_connection (GVfsDBusDaemon *object,
                       GDBusMethodInvocation *invocation,
                       gpointer user_data)
{
  GVfsDaemon *daemon = G_VFS_DAEMON (user_data);
  GDBusServer *server;
  GError *error;
  gchar *address1;
  NewConnectionData *data;
  char *socket_dir;
  gchar *guid;
  
  generate_address (&address1, &socket_dir);

  data = g_new (NewConnectionData, 1);
  data->daemon = daemon;
  data->socket_dir = socket_dir;
  data->conn = NULL;

  guid = g_dbus_generate_guid ();
  error = NULL;
  server = g_dbus_server_new_sync (address1,
                                   G_DBUS_SERVER_FLAGS_NONE,
                                   guid,
                                   NULL, /* GDBusAuthObserver */
                                   NULL, /* GCancellable */
                                   &error);
  g_free (guid);

  if (server == NULL)
    {
      g_dbus_method_invocation_return_gerror (invocation, error);
      g_printerr ("daemon: Error creating server at address %s: %s\n", address1, error->message);
      g_error_free (error);
      goto error_out;
    }

  g_dbus_server_start (server);
  data->server = server;

  g_signal_connect (server, "new-connection", G_CALLBACK (daemon_new_connection_func), data);
  
  gvfs_dbus_daemon_complete_get_connection (object,
                                            invocation,
                                            address1,
                                            "");

  g_free (address1);
  return TRUE;

 error_out:
  new_connection_data_free (data);
  g_free (address1);
  return TRUE;
}