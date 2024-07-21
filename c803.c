bgp_capability_as4 (struct peer *peer, struct capability_header *hdr)
{
  as_t as4 = stream_getl (BGP_INPUT(peer));
  
  if (BGP_DEBUG (as4, AS4))
    zlog_debug ("%s [AS4] about to set cap PEER_CAP_AS4_RCV, got as4 %u",
                peer->host, as4);
  SET_FLAG (peer->cap, PEER_CAP_AS4_RCV);
  
  return as4;
}