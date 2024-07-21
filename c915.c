deregister_application (SpiBridge * app)
{
  DBusMessage *message;
  DBusMessageIter iter;
  DBusError error;
  const char *uname;

  dbus_error_init (&error);

  message = dbus_message_new_method_call (SPI_DBUS_NAME_REGISTRY,
                                          ATSPI_DBUS_PATH_REGISTRY,
                                          ATSPI_DBUS_INTERFACE_REGISTRY,
                                          "DeregisterApplication");
  dbus_message_set_no_reply (message, TRUE);

  uname = dbus_bus_get_unique_name (app->bus);

  dbus_message_iter_init_append (message, &iter);
  dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &uname);
  dbus_connection_send (app->bus, message, NULL);
  if (message)
    dbus_message_unref (message);
}