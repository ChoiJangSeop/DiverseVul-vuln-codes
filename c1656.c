bgp_attr_ext_communities (struct peer *peer, bgp_size_t length, 
			  struct attr *attr, u_char flag, u_char *startp)
{
  bgp_size_t total
    = length + (CHECK_FLAG (flag, BGP_ATTR_FLAG_EXTLEN) ? 4 : 3);
  
  if (length == 0)
    {
      if (attr->extra)
        attr->extra->ecommunity = NULL;
      /* Empty extcomm doesn't seem to be invalid per se */
      return BGP_ATTR_PARSE_PROCEED;
    }

  /* Flags check. */
  if (bgp_attr_flag_invalid (peer, BGP_ATTR_EXT_COMMUNITIES, flag))
    return bgp_attr_malformed (peer, BGP_ATTR_EXT_COMMUNITIES, flag,
                               BGP_NOTIFY_UPDATE_ATTR_FLAG_ERR,
                               startp, total);
  
  (bgp_attr_extra_get (attr))->ecommunity =
    ecommunity_parse ((u_int8_t *)stream_pnt (peer->ibuf), length);
  /* XXX: fix ecommunity_parse to use stream API */
  stream_forward_getp (peer->ibuf, length);
  
  if (!attr->extra->ecommunity)
    return bgp_attr_malformed (peer, BGP_ATTR_EXT_COMMUNITIES,
                               flag, BGP_NOTIFY_UPDATE_OPT_ATTR_ERR,
                               startp, total);
  
  attr->flag |= ATTR_FLAG_BIT (BGP_ATTR_EXT_COMMUNITIES);

  return BGP_ATTR_PARSE_PROCEED;
}