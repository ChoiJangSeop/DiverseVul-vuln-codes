g_socket_client_tls_handshake (GSocketClientAsyncConnectData *data)
{
  GIOStream *tlsconn;

  if (!data->client->priv->tls)
    {
      g_socket_client_async_connect_complete (data);
      return;
    }

  tlsconn = g_tls_client_connection_new (data->connection,
					 data->connectable,
					 &data->last_error);
  if (tlsconn)
    {
      g_tls_client_connection_set_validation_flags (G_TLS_CLIENT_CONNECTION (tlsconn),
                                                    data->client->priv->tls_validation_flags);
      g_socket_client_emit_event (data->client, G_SOCKET_CLIENT_TLS_HANDSHAKING, data->connectable, G_IO_STREAM (tlsconn));
      g_tls_connection_handshake_async (G_TLS_CONNECTION (tlsconn),
					G_PRIORITY_DEFAULT,
					g_task_get_cancellable (data->task),
					g_socket_client_tls_handshake_callback,
					data);
    }
  else
    {
      enumerator_next_async (data);
    }
}