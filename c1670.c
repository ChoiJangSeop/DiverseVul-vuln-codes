bgp_attr_community (struct peer *peer, bgp_size_t length, 
		    struct attr *attr, u_char flag, u_char *startp)
{
  bgp_size_t total
    = length + (CHECK_FLAG (flag, BGP_ATTR_FLAG_EXTLEN) ? 4 : 3);
  
  if (length == 0)
    {
      attr->community = NULL;
      return BGP_ATTR_PARSE_PROCEED;
    }
  
  /* Flags check. */
  if (bgp_attr_flag_invalid (peer, BGP_ATTR_COMMUNITIES, flag))
    return bgp_attr_malformed (peer, BGP_ATTR_COMMUNITIES, flag,
                               BGP_NOTIFY_UPDATE_ATTR_FLAG_ERR,
                               startp, total);
  
  attr->community =
    community_parse ((u_int32_t *)stream_pnt (peer->ibuf), length);
  
  /* XXX: fix community_parse to use stream API and remove this */
  stream_forward_getp (peer->ibuf, length);

  if (!attr->community)
    return bgp_attr_malformed (peer, BGP_ATTR_COMMUNITIES, flag,
                               BGP_NOTIFY_UPDATE_OPT_ATTR_ERR,
                               startp, total);
  
  attr->flag |= ATTR_FLAG_BIT (BGP_ATTR_COMMUNITIES);

  return BGP_ATTR_PARSE_PROCEED;
}