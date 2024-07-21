bgp_attr_aspath (struct peer *peer, bgp_size_t length, 
		 struct attr *attr, u_char flag, u_char *startp)
{
  bgp_size_t total;

  total = length + (CHECK_FLAG (flag, BGP_ATTR_FLAG_EXTLEN) ? 4 : 3);
  
  if (bgp_attr_flag_invalid (peer, BGP_ATTR_AS_PATH, flag))
      return bgp_attr_malformed (peer, BGP_ATTR_AS_PATH, flag,
				 BGP_NOTIFY_UPDATE_ATTR_FLAG_ERR,
				 startp, total);
  /*
   * peer with AS4 => will get 4Byte ASnums
   * otherwise, will get 16 Bit
   */
  attr->aspath = aspath_parse (peer->ibuf, length, 
                               CHECK_FLAG (peer->cap, PEER_CAP_AS4_RCV));

  /* In case of IBGP, length will be zero. */
  if (! attr->aspath)
    {
      zlog (peer->log, LOG_ERR,
            "Malformed AS path from %s, length is %d",
            peer->host, length);
      return bgp_attr_malformed (peer, BGP_ATTR_AS_PATH, flag,
                                 BGP_NOTIFY_UPDATE_MAL_AS_PATH,
                                 NULL, 0);
    }

  /* Set aspath attribute flag. */
  attr->flag |= ATTR_FLAG_BIT (BGP_ATTR_AS_PATH);

  return BGP_ATTR_PARSE_PROCEED;
}