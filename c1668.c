bgp_attr_aggregator (struct peer *peer, bgp_size_t length,
		     struct attr *attr, u_char flag, u_char *startp)
{
  int wantedlen = 6;
  struct attr_extra *attre = bgp_attr_extra_get (attr);
  bgp_size_t total;
  
  total = length + (CHECK_FLAG (flag, BGP_ATTR_FLAG_EXTLEN) ? 4 : 3);
  /* Flags check. */
  if (bgp_attr_flag_invalid (peer, BGP_ATTR_AGGREGATOR, flag))
    return bgp_attr_malformed (peer, BGP_ATTR_AGGREGATOR, flag,
                               BGP_NOTIFY_UPDATE_ATTR_FLAG_ERR,
                               startp, total);
  
  /* peer with AS4 will send 4 Byte AS, peer without will send 2 Byte */
  if (CHECK_FLAG (peer->cap, PEER_CAP_AS4_RCV))
    wantedlen = 8;
  
  if (length != wantedlen)
    {
      zlog (peer->log, LOG_ERR, "AGGREGATOR attribute length isn't %u [%u]", wantedlen, length);
      return bgp_attr_malformed (peer, BGP_ATTR_AGGREGATOR, flag,
                                 BGP_NOTIFY_UPDATE_ATTR_LENG_ERR,
                                 startp, total);
    }
  
  if ( CHECK_FLAG (peer->cap, PEER_CAP_AS4_RCV ) )
    attre->aggregator_as = stream_getl (peer->ibuf);
  else
    attre->aggregator_as = stream_getw (peer->ibuf);
  attre->aggregator_addr.s_addr = stream_get_ipv4 (peer->ibuf);

  /* Set atomic aggregate flag. */
  attr->flag |= ATTR_FLAG_BIT (BGP_ATTR_AGGREGATOR);

  return BGP_ATTR_PARSE_PROCEED;
}