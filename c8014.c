backend_set_handle (struct backend *b, struct connection *conn, void *handle)
{
  assert (b->i < conn->nr_handles);
  conn->handles[b->i].handle = handle;
}