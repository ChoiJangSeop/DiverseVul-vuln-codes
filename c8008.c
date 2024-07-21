protocol_handshake (struct connection *conn)
{
  int r;

  lock_request (conn);
  if (!newstyle)
    r = protocol_handshake_oldstyle (conn);
  else
    r = protocol_handshake_newstyle (conn);
  unlock_request (conn);

  return r;
}