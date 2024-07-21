g_socket_client_enumerator_callback (GObject      *object,
				     GAsyncResult *result,
				     gpointer      user_data)
{
  GSocketClientAsyncConnectData *data = user_data;
  GSocketAddress *address = NULL;
  GSocket *socket;
  ConnectionAttempt *attempt;
  GError *error = NULL;

  if (g_task_return_error_if_cancelled (data->task))
    {
      g_object_unref (data->task);
      return;
    }

  address = g_socket_address_enumerator_next_finish (data->enumerator,
						     result, &error);
  if (address == NULL)
    {
      if (data->connection_attempts)
        return;

      g_socket_client_emit_event (data->client, G_SOCKET_CLIENT_COMPLETE, data->connectable, NULL);
      if (!error)
	{
	  if (data->last_error)
	    {
	      error = data->last_error;
	      data->last_error = NULL;
	    }
	  else
	    {
	      g_set_error_literal (&error, G_IO_ERROR, G_IO_ERROR_FAILED,
				   _("Unknown error on connect"));
	    }
	}
      g_task_return_error (data->task, error);
      g_object_unref (data->task);
      return;
    }

  g_socket_client_emit_event (data->client, G_SOCKET_CLIENT_RESOLVED,
			      data->connectable, NULL);

  if (G_IS_PROXY_ADDRESS (address) &&
      data->client->priv->enable_proxy)
    data->proxy_addr = g_object_ref (G_PROXY_ADDRESS (address));

  g_clear_error (&data->last_error);

  socket = create_socket (data->client, address, &data->last_error);
  if (socket == NULL)
    {
      g_object_unref (address);
      enumerator_next_async (data);
      return;
    }

  attempt = connection_attempt_new ();
  attempt->data = data;
  attempt->socket = socket;
  attempt->address = address;
  attempt->cancellable = g_cancellable_new ();
  attempt->connection = (GIOStream *)g_socket_connection_factory_create_connection (socket);
  attempt->timeout_source = g_timeout_source_new (HAPPY_EYEBALLS_CONNECTION_ATTEMPT_TIMEOUT_MS);
  g_source_set_callback (attempt->timeout_source, on_connection_attempt_timeout, attempt, NULL);
  g_source_attach (attempt->timeout_source, g_main_context_get_thread_default ());
  data->connection_attempts = g_slist_append (data->connection_attempts, attempt);

  if (g_task_get_cancellable (data->task))
    g_cancellable_connect (g_task_get_cancellable (data->task), G_CALLBACK (on_connection_cancelled),
                           g_object_ref (attempt->cancellable), g_object_unref);

  g_socket_connection_set_cached_remote_address ((GSocketConnection *)attempt->connection, address);
  g_socket_client_emit_event (data->client, G_SOCKET_CLIENT_CONNECTING, data->connectable, attempt->connection);
  g_socket_connection_connect_async (G_SOCKET_CONNECTION (attempt->connection),
				     address,
				     attempt->cancellable,
				     g_socket_client_connected_callback, connection_attempt_ref (attempt));
}