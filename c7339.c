enumerator_next_async (GSocketClientAsyncConnectData *data)
{
  /* We need to cleanup the state */
  g_clear_object (&data->socket);
  g_clear_object (&data->proxy_addr);
  g_clear_object (&data->connection);

  g_socket_client_emit_event (data->client, G_SOCKET_CLIENT_RESOLVING, data->connectable, NULL);
  g_socket_address_enumerator_next_async (data->enumerator,
					  g_task_get_cancellable (data->task),
					  g_socket_client_enumerator_callback,
					  data);
}