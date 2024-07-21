plugin_close (struct backend *b, struct connection *conn)
{
  struct backend_plugin *p = container_of (b, struct backend_plugin, backend);

  assert (connection_get_handle (conn, 0));

  debug ("close");

  if (p->plugin.close)
    p->plugin.close (connection_get_handle (conn, 0));

  backend_set_handle (b, conn, NULL);
}