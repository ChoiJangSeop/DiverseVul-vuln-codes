int mutt_ssl_starttls (CONNECTION* conn)
{
  if (tls_init() < 0)
    return -1;

  if (tls_negotiate (conn) < 0)
    return -1;

  conn->conn_read	= tls_socket_read;
  conn->conn_write	= tls_socket_write;
  conn->conn_close	= tls_starttls_close;
  conn->conn_poll       = tls_socket_poll;

  return 0;
}