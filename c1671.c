bgp_attr_parse (struct peer *peer, struct attr *attr, bgp_size_t size,
		struct bgp_nlri *mp_update, struct bgp_nlri *mp_withdraw)
{
  int ret;
  u_char flag = 0;
  u_char type = 0;
  bgp_size_t length;
  u_char *startp, *endp;
  u_char *attr_endp;
  u_char seen[BGP_ATTR_BITMAP_SIZE];
  /* we need the as4_path only until we have synthesized the as_path with it */
  /* same goes for as4_aggregator */
  struct aspath *as4_path = NULL;
  as_t as4_aggregator = 0;
  struct in_addr as4_aggregator_addr = { 0 };

  /* Initialize bitmap. */
  memset (seen, 0, BGP_ATTR_BITMAP_SIZE);

  /* End pointer of BGP attribute. */
  endp = BGP_INPUT_PNT (peer) + size;
  
  /* Get attributes to the end of attribute length. */
  while (BGP_INPUT_PNT (peer) < endp)
    {
      /* Check remaining length check.*/
      if (endp - BGP_INPUT_PNT (peer) < BGP_ATTR_MIN_LEN)
	{
	  /* XXX warning: long int format, int arg (arg 5) */
	  zlog (peer->log, LOG_WARNING, 
		"%s: error BGP attribute length %lu is smaller than min len",
		peer->host,
		(unsigned long) (endp - STREAM_PNT (BGP_INPUT (peer))));

	  bgp_notify_send (peer, 
			   BGP_NOTIFY_UPDATE_ERR, 
			   BGP_NOTIFY_UPDATE_ATTR_LENG_ERR);
	  return BGP_ATTR_PARSE_ERROR;
	}

      /* Fetch attribute flag and type. */
      startp = BGP_INPUT_PNT (peer);
      /* "The lower-order four bits of the Attribute Flags octet are
         unused.  They MUST be zero when sent and MUST be ignored when
         received." */
      flag = 0xF0 & stream_getc (BGP_INPUT (peer));
      type = stream_getc (BGP_INPUT (peer));

      /* Check whether Extended-Length applies and is in bounds */
      if (CHECK_FLAG (flag, BGP_ATTR_FLAG_EXTLEN)
          && ((endp - startp) < (BGP_ATTR_MIN_LEN + 1)))
	{
	  zlog (peer->log, LOG_WARNING, 
		"%s: Extended length set, but just %lu bytes of attr header",
		peer->host,
		(unsigned long) (endp - STREAM_PNT (BGP_INPUT (peer))));

	  bgp_notify_send (peer, 
			   BGP_NOTIFY_UPDATE_ERR, 
			   BGP_NOTIFY_UPDATE_ATTR_LENG_ERR);
	  return BGP_ATTR_PARSE_ERROR;
	}

      /* Check extended attribue length bit. */
      if (CHECK_FLAG (flag, BGP_ATTR_FLAG_EXTLEN))
	length = stream_getw (BGP_INPUT (peer));
      else
	length = stream_getc (BGP_INPUT (peer));
      
      /* If any attribute appears more than once in the UPDATE
	 message, then the Error Subcode is set to Malformed Attribute
	 List. */

      if (CHECK_BITMAP (seen, type))
	{
	  zlog (peer->log, LOG_WARNING,
		"%s: error BGP attribute type %d appears twice in a message",
		peer->host, type);

	  bgp_notify_send (peer, 
			   BGP_NOTIFY_UPDATE_ERR, 
			   BGP_NOTIFY_UPDATE_MAL_ATTR);
	  return BGP_ATTR_PARSE_ERROR;
	}

      /* Set type to bitmap to check duplicate attribute.  `type' is
	 unsigned char so it never overflow bitmap range. */

      SET_BITMAP (seen, type);

      /* Overflow check. */
      attr_endp =  BGP_INPUT_PNT (peer) + length;

      if (attr_endp > endp)
	{
	  zlog (peer->log, LOG_WARNING, 
		"%s: BGP type %d length %d is too large, attribute total length is %d.  attr_endp is %p.  endp is %p", peer->host, type, length, size, attr_endp, endp);
	  bgp_notify_send (peer, 
			   BGP_NOTIFY_UPDATE_ERR, 
			   BGP_NOTIFY_UPDATE_ATTR_LENG_ERR);
	  return BGP_ATTR_PARSE_ERROR;
	}

      /* OK check attribute and store it's value. */
      switch (type)
	{
	case BGP_ATTR_ORIGIN:
	  ret = bgp_attr_origin (peer, length, attr, flag, startp);
	  break;
	case BGP_ATTR_AS_PATH:
	  ret = bgp_attr_aspath (peer, length, attr, flag, startp);
	  break;
	case BGP_ATTR_AS4_PATH:
	  ret = bgp_attr_as4_path (peer, length, attr, flag, startp, &as4_path);
	  break;
	case BGP_ATTR_NEXT_HOP:	
	  ret = bgp_attr_nexthop (peer, length, attr, flag, startp);
	  break;
	case BGP_ATTR_MULTI_EXIT_DISC:
	  ret = bgp_attr_med (peer, length, attr, flag, startp);
	  break;
	case BGP_ATTR_LOCAL_PREF:
	  ret = bgp_attr_local_pref (peer, length, attr, flag, startp);
	  break;
	case BGP_ATTR_ATOMIC_AGGREGATE:
	  ret = bgp_attr_atomic (peer, length, attr, flag, startp);
	  break;
	case BGP_ATTR_AGGREGATOR:
	  ret = bgp_attr_aggregator (peer, length, attr, flag, startp);
	  break;
	case BGP_ATTR_AS4_AGGREGATOR:
	  ret = bgp_attr_as4_aggregator (peer, length, attr, flag,
	                                 &as4_aggregator, 
	                                 &as4_aggregator_addr, startp);
	  break;
	case BGP_ATTR_COMMUNITIES:
	  ret = bgp_attr_community (peer, length, attr, flag, startp);
	  break;
	case BGP_ATTR_ORIGINATOR_ID:
	  ret = bgp_attr_originator_id (peer, length, attr, flag, startp);
	  break;
	case BGP_ATTR_CLUSTER_LIST:
	  ret = bgp_attr_cluster_list (peer, length, attr, flag, startp);
	  break;
	case BGP_ATTR_MP_REACH_NLRI:
	  ret = bgp_mp_reach_parse (peer, length, attr, flag, startp, mp_update);
	  break;
	case BGP_ATTR_MP_UNREACH_NLRI:
	  ret = bgp_mp_unreach_parse (peer, length, flag, startp, mp_withdraw);
	  break;
	case BGP_ATTR_EXT_COMMUNITIES:
	  ret = bgp_attr_ext_communities (peer, length, attr, flag, startp);
	  break;
	default:
	  ret = bgp_attr_unknown (peer, attr, flag, type, length, startp);
	  break;
	}
      
      /* If hard error occured immediately return to the caller. */
      if (ret == BGP_ATTR_PARSE_ERROR)
        {
          zlog (peer->log, LOG_WARNING,
                "%s: Attribute %s, parse error", 
                peer->host, 
                LOOKUP (attr_str, type));
          bgp_notify_send (peer, 
                           BGP_NOTIFY_UPDATE_ERR,
                           BGP_NOTIFY_UPDATE_MAL_ATTR);
          if (as4_path)
            aspath_unintern (&as4_path);
          return ret;
        }
      if (ret == BGP_ATTR_PARSE_WITHDRAW)
        {
          
          zlog (peer->log, LOG_WARNING,
                "%s: Attribute %s, parse error - treating as withdrawal",
                peer->host,
                LOOKUP (attr_str, type));
          if (as4_path)
            aspath_unintern (&as4_path);
          return ret;
        }
      
      /* Check the fetched length. */
      if (BGP_INPUT_PNT (peer) != attr_endp)
	{
	  zlog (peer->log, LOG_WARNING, 
		"%s: BGP attribute %s, fetch error", 
                peer->host, LOOKUP (attr_str, type));
	  bgp_notify_send (peer, 
			   BGP_NOTIFY_UPDATE_ERR, 
			   BGP_NOTIFY_UPDATE_ATTR_LENG_ERR);
          if (as4_path)
            aspath_unintern (&as4_path);
	  return BGP_ATTR_PARSE_ERROR;
	}
    }

  /* Check final read pointer is same as end pointer. */
  if (BGP_INPUT_PNT (peer) != endp)
    {
      zlog (peer->log, LOG_WARNING, 
	    "%s: BGP attribute %s, length mismatch",
	    peer->host, LOOKUP (attr_str, type));
      bgp_notify_send (peer, 
		       BGP_NOTIFY_UPDATE_ERR, 
		       BGP_NOTIFY_UPDATE_ATTR_LENG_ERR);
      if (as4_path)
        aspath_unintern (&as4_path);
      return BGP_ATTR_PARSE_ERROR;
    }

  /* 
   * At this place we can see whether we got AS4_PATH and/or
   * AS4_AGGREGATOR from a 16Bit peer and act accordingly.
   * We can not do this before we've read all attributes because
   * the as4 handling does not say whether AS4_PATH has to be sent
   * after AS_PATH or not - and when AS4_AGGREGATOR will be send
   * in relationship to AGGREGATOR.
   * So, to be defensive, we are not relying on any order and read
   * all attributes first, including these 32bit ones, and now,
   * afterwards, we look what and if something is to be done for as4.
   */
  if (bgp_attr_munge_as4_attrs (peer, attr, flag, as4_path,
                                as4_aggregator, &as4_aggregator_addr))
    {
      if (as4_path)
        aspath_unintern (&as4_path);
      return BGP_ATTR_PARSE_ERROR;
    }

  /* At this stage, we have done all fiddling with as4, and the
   * resulting info is in attr->aggregator resp. attr->aspath
   * so we can chuck as4_aggregator and as4_path alltogether in
   * order to save memory
   */
  if (as4_path)
    {
      aspath_unintern (&as4_path); /* unintern - it is in the hash */
      /* The flag that we got this is still there, but that does not
       * do any trouble
       */
    }
  /*
   * The "rest" of the code does nothing with as4_aggregator.
   * there is no memory attached specifically which is not part
   * of the attr.
   * so ignoring just means do nothing.
   */
  /*
   * Finally do the checks on the aspath we did not do yet
   * because we waited for a potentially synthesized aspath.
   */
  if (attr->flag & (ATTR_FLAG_BIT(BGP_ATTR_AS_PATH)))
    {
      ret = bgp_attr_aspath_check (peer, attr, flag);
      if (ret != BGP_ATTR_PARSE_PROCEED)
	return ret;
    }

  /* Finally intern unknown attribute. */
  if (attr->extra && attr->extra->transit)
    attr->extra->transit = transit_intern (attr->extra->transit);

  return BGP_ATTR_PARSE_PROCEED;
}