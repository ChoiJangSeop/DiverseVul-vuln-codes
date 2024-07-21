g_socket_client_tls_handshake_callback (GObject      *object,
					GAsyncResult *result,
					gpointer      user_data)
{
  GSocketClientAsyncConnectData *data = user_data;

  if (g_tls_connection_handshake_finish (G_TLS_CONNECTION (object),
					 result,
					 &data->last_error))
    {
      g_object_unref (data->connection);
      data->connection = G_IO_STREAM (object);

      g_socket_client_emit_event (data->client, G_SOCKET_CLIENT_TLS_HANDSHAKED, data->connectable, data->connection);
      g_socket_client_async_connect_complete (data);
    }
  else
    {
      g_object_unref (object);
      enumerator_next_async (data);
    }
}