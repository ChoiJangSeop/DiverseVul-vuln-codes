on_connection_attempt_timeout (gpointer data)
{
  ConnectionAttempt *attempt = data;

  enumerator_next_async (attempt->data);

  g_clear_pointer (&attempt->timeout_source, g_source_unref);
  return G_SOURCE_REMOVE;
}