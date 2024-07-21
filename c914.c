impl_get_app_bus(DBusConnection *bus, DBusMessage *msg, void *data)
{
DBusMessage *reply;

  if (bus == spi_global_app_data->bus)
    spi_atk_add_client (dbus_message_get_sender (msg));

reply = dbus_message_new_method_return(msg);
if (reply)
    {
      const char *retval = (g_getenv ("AT_SPI_CLIENT") ?
                            "":
                            spi_global_app_data->app_bus_addr);
      dbus_message_append_args(reply, DBUS_TYPE_STRING, &retval, DBUS_TYPE_INVALID);
    }

return reply;
}