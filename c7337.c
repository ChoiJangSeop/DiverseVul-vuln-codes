g_socket_client_async_connect_data_free (GSocketClientAsyncConnectData *data)
{
  g_clear_object (&data->connectable);
  g_clear_object (&data->enumerator);
  g_clear_object (&data->proxy_addr);
  g_clear_object (&data->socket);
  g_clear_object (&data->connection);
  g_slist_free_full (data->connection_attempts, connection_attempt_unref);

  g_clear_error (&data->last_error);

  g_slice_free (GSocketClientAsyncConnectData, data);
}