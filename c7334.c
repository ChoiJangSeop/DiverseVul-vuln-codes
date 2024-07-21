g_socket_client_proxy_connect_callback (GObject      *object,
					GAsyncResult *result,
					gpointer      user_data)
{
  GSocketClientAsyncConnectData *data = user_data;

  g_object_unref (data->connection);
  data->connection = g_proxy_connect_finish (G_PROXY (object),
					     result,
					     &data->last_error);
  if (data->connection)
    {
      g_socket_client_emit_event (data->client, G_SOCKET_CLIENT_PROXY_NEGOTIATED, data->connectable, data->connection);
    }
  else
    {
      enumerator_next_async (data);
      return;
    }

  g_socket_client_tls_handshake (data);
}