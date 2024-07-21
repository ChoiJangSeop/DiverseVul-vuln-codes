bgp_attr_atomic (struct peer *peer, bgp_size_t length, 
		 struct attr *attr, u_char flag, u_char *startp)
{
  bgp_size_t total;

  total = length + (CHECK_FLAG (flag, BGP_ATTR_FLAG_EXTLEN) ? 4 : 3);
  /* Flag checks. */
  if (bgp_attr_flag_invalid (peer, BGP_ATTR_ATOMIC_AGGREGATE, flag))
    return bgp_attr_malformed (peer, BGP_ATTR_ATOMIC_AGGREGATE, flag,
                               BGP_NOTIFY_UPDATE_ATTR_FLAG_ERR,
                               startp, total);
  
  /* Length check. */
  if (length != 0)
    {
      zlog (peer->log, LOG_ERR, "ATOMIC_AGGREGATE attribute length isn't 0 [%u]", length);
      return bgp_attr_malformed (peer, BGP_ATTR_ATOMIC_AGGREGATE, flag,
                                 BGP_NOTIFY_UPDATE_ATTR_LENG_ERR,
                                 startp, total);
    }

  /* Set atomic aggregate flag. */
  attr->flag |= ATTR_FLAG_BIT (BGP_ATTR_ATOMIC_AGGREGATE);

  return BGP_ATTR_PARSE_PROCEED;
}