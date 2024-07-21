filter_close (struct backend *b, struct connection *conn)
{
  struct backend_filter *f = container_of (b, struct backend_filter, backend);
  void *handle = connection_get_handle (conn, b->i);

  debug ("%s: close", b->name);

  if (handle && f->filter.close)
    f->filter.close (handle);
  backend_set_handle (b, conn, NULL);
  b->next->close (b->next, conn);
}