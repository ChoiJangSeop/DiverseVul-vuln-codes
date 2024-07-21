setup_bus (void)
{
#ifndef DISABLE_P2P
  DBusServer *server;
  DBusError err;

  dbus_error_init(&err);
  server = dbus_server_listen(spi_global_app_data->app_bus_addr, &err);
  if (server == NULL)
  {
    g_warning ("atk-bridge: Couldn't listen on dbus server: %s", err.message);
    dbus_error_init (&err);
    spi_global_app_data->app_bus_addr [0] = '\0';
    g_main_context_unref (spi_global_app_data->main_context);
    spi_global_app_data->main_context = NULL;
    return -1;
  }

  atspi_dbus_server_setup_with_g_main(server, NULL);
  dbus_server_set_new_connection_function(server, new_connection_cb, NULL, NULL);

  spi_global_app_data->server = server;
#endif

  return 0;
}