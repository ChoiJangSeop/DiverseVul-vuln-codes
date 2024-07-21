bgp_attr_originator_id (struct peer *peer, bgp_size_t length, 
			struct attr *attr, u_char flag, u_char *startp)
{
  bgp_size_t total;

  total = length + (CHECK_FLAG (flag, BGP_ATTR_FLAG_EXTLEN) ? 4 : 3);
  /* Flag checks. */
  if (bgp_attr_flag_invalid (peer, BGP_ATTR_ORIGINATOR_ID, flag))
    return bgp_attr_malformed (peer, BGP_ATTR_ORIGINATOR_ID, flag,
                               BGP_NOTIFY_UPDATE_ATTR_FLAG_ERR,
                               startp, total);
  /* Length check. */
  if (length != 4)
    {
      zlog (peer->log, LOG_ERR, "Bad originator ID length %d", length);

      return bgp_attr_malformed (peer, BGP_ATTR_ORIGINATOR_ID, flag,
                                 BGP_NOTIFY_UPDATE_ATTR_LENG_ERR,
                                 startp, total);
    }

  (bgp_attr_extra_get (attr))->originator_id.s_addr 
    = stream_get_ipv4 (peer->ibuf);

  attr->flag |= ATTR_FLAG_BIT (BGP_ATTR_ORIGINATOR_ID);

  return BGP_ATTR_PARSE_PROCEED;
}