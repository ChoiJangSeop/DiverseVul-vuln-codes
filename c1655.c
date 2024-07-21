bgp_attr_as4_aggregator (struct peer *peer, bgp_size_t length,
		     struct attr *attr, u_char flag, 
		     as_t *as4_aggregator_as,
		     struct in_addr *as4_aggregator_addr,
		     u_char *startp)
{
  bgp_size_t total;
  
  if (length != 8)
    {
      zlog (peer->log, LOG_ERR, "New Aggregator length is not 8 [%d]", length);
      return bgp_attr_malformed (peer, BGP_ATTR_AS4_AGGREGATOR, flag,
                                 BGP_NOTIFY_UPDATE_ATTR_LENG_ERR,
                                 NULL, 0);
    }
  total = length + (CHECK_FLAG (flag, BGP_ATTR_FLAG_EXTLEN) ? 4 : 3);
  /* Flags check. */
  if (bgp_attr_flag_invalid (peer, BGP_ATTR_AS4_AGGREGATOR, flag))
    return bgp_attr_malformed (peer, BGP_ATTR_AS4_AGGREGATOR, flag,
                               BGP_NOTIFY_UPDATE_ATTR_FLAG_ERR,
                               startp, total);
  
  *as4_aggregator_as = stream_getl (peer->ibuf);
  as4_aggregator_addr->s_addr = stream_get_ipv4 (peer->ibuf);

  attr->flag |= ATTR_FLAG_BIT (BGP_ATTR_AS4_AGGREGATOR);

  return BGP_ATTR_PARSE_PROCEED;
}