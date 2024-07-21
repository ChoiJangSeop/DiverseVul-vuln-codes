register_application (SpiBridge * app)
{
  DBusMessage *message;
  DBusMessageIter iter;
  DBusError error;
  DBusPendingCall *pending;
  const int max_addr_length = 128; /* should be long enough */

  dbus_error_init (&error);

  /* These will be overridden when we get a reply, but in practice these
     defaults should always be correct */
  app->desktop_name = ATSPI_DBUS_NAME_REGISTRY;
  app->desktop_path = ATSPI_DBUS_PATH_ROOT;

  message = dbus_message_new_method_call (SPI_DBUS_NAME_REGISTRY,
                                          ATSPI_DBUS_PATH_ROOT,
                                          ATSPI_DBUS_INTERFACE_SOCKET,
                                          "Embed");

  dbus_message_iter_init_append (message, &iter);
  spi_object_append_reference (&iter, app->root);
  
    if (!dbus_connection_send_with_reply (app->bus, message, &pending, -1)
        || !pending)
    {
        return FALSE;
    }

    dbus_pending_call_set_notify (pending, register_reply, app, NULL);

  if (message)
    dbus_message_unref (message);

  /* could this be better, we accept some amount of race in getting the temp name*/
  /* make sure the directory exists */
  mkdir ("/tmp/at-spi2/", S_IRWXU|S_IRWXG|S_IRWXO|S_ISVTX);
  chmod ("/tmp/at-spi2/", S_IRWXU|S_IRWXG|S_IRWXO|S_ISVTX);
  app->app_bus_addr = g_malloc(max_addr_length * sizeof(char));
#ifndef DISABLE_P2P
  sprintf (app->app_bus_addr, "unix:path=/tmp/at-spi2/socket-%d-%d", getpid(),
           rand());
#else
  app->app_bus_addr [0] = '\0';
#endif

  return TRUE;
}