bgp_auth_parse (struct peer *peer, u_char *pnt, size_t length)
{
  bgp_notify_send (peer, 
		   BGP_NOTIFY_OPEN_ERR, 
		   BGP_NOTIFY_OPEN_AUTH_FAILURE); 
  return -1;
}