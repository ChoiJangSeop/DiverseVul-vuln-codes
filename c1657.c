bgp_mp_reach_parse (struct peer *peer, const bgp_size_t length,
                    struct attr *attr, const u_char flag, u_char *startp, struct bgp_nlri *mp_update)
{
  afi_t afi;
  safi_t safi;
  bgp_size_t nlri_len;
  size_t start;
  int ret;
  struct stream *s;
  struct attr_extra *attre = bgp_attr_extra_get(attr);
  bgp_size_t total;

  total = length + (CHECK_FLAG (flag, BGP_ATTR_FLAG_EXTLEN) ? 4 : 3);
  /* Flag checks. */
  if (bgp_attr_flag_invalid (peer, BGP_ATTR_MP_REACH_NLRI, flag))
    return bgp_attr_malformed (peer, BGP_ATTR_MP_REACH_NLRI, flag,
                               BGP_NOTIFY_UPDATE_ATTR_FLAG_ERR,
                               startp, total);
  /* Set end of packet. */
  s = BGP_INPUT(peer);
  start = stream_get_getp(s);
  
  /* safe to read statically sized header? */
#define BGP_MP_REACH_MIN_SIZE 5
#define LEN_LEFT	(length - (stream_get_getp(s) - start))
  if ((length > STREAM_READABLE(s)) || (length < BGP_MP_REACH_MIN_SIZE))
    {
      zlog_info ("%s: %s sent invalid length, %lu", 
		 __func__, peer->host, (unsigned long)length);
      return BGP_ATTR_PARSE_ERROR;
    }
  
  /* Load AFI, SAFI. */
  afi = stream_getw (s);
  safi = stream_getc (s);

  /* Get nexthop length. */
  attre->mp_nexthop_len = stream_getc (s);
  
  if (LEN_LEFT < attre->mp_nexthop_len)
    {
      zlog_info ("%s: %s, MP nexthop length, %u, goes past end of attribute", 
		 __func__, peer->host, attre->mp_nexthop_len);
      return BGP_ATTR_PARSE_ERROR;
    }
  
  /* Nexthop length check. */
  switch (attre->mp_nexthop_len)
    {
    case 4:
      stream_get (&attre->mp_nexthop_global_in, s, 4);
      /* Probably needed for RFC 2283 */
      if (attr->nexthop.s_addr == 0)
        memcpy(&attr->nexthop.s_addr, &attre->mp_nexthop_global_in, 4);
      break;
    case 12:
      stream_getl (s); /* RD high */
      stream_getl (s); /* RD low */
      stream_get (&attre->mp_nexthop_global_in, s, 4);
      break;
#ifdef HAVE_IPV6
    case 16:
      stream_get (&attre->mp_nexthop_global, s, 16);
      break;
    case 32:
      stream_get (&attre->mp_nexthop_global, s, 16);
      stream_get (&attre->mp_nexthop_local, s, 16);
      if (! IN6_IS_ADDR_LINKLOCAL (&attre->mp_nexthop_local))
	{
	  char buf1[INET6_ADDRSTRLEN];
	  char buf2[INET6_ADDRSTRLEN];

	  if (BGP_DEBUG (update, UPDATE_IN))
	    zlog_debug ("%s got two nexthop %s %s but second one is not a link-local nexthop", peer->host,
		       inet_ntop (AF_INET6, &attre->mp_nexthop_global,
				  buf1, INET6_ADDRSTRLEN),
		       inet_ntop (AF_INET6, &attre->mp_nexthop_local,
				  buf2, INET6_ADDRSTRLEN));

	  attre->mp_nexthop_len = 16;
	}
      break;
#endif /* HAVE_IPV6 */
    default:
      zlog_info ("%s: (%s) Wrong multiprotocol next hop length: %d", 
		 __func__, peer->host, attre->mp_nexthop_len);
      return BGP_ATTR_PARSE_ERROR;
    }

  if (!LEN_LEFT)
    {
      zlog_info ("%s: (%s) Failed to read SNPA and NLRI(s)",
                 __func__, peer->host);
      return BGP_ATTR_PARSE_ERROR;
    }
  
  {
    u_char val; 
    if ((val = stream_getc (s)))
    zlog_warn ("%s sent non-zero value, %u, for defunct SNPA-length field",
                peer->host, val);
  }
  
  /* must have nrli_len, what is left of the attribute */
  nlri_len = LEN_LEFT;
  if ((!nlri_len) || (nlri_len > STREAM_READABLE(s)))
    {
      zlog_info ("%s: (%s) Failed to read NLRI",
                 __func__, peer->host);
      return BGP_ATTR_PARSE_ERROR;
    }
 
  if (safi != SAFI_MPLS_LABELED_VPN)
    {
      ret = bgp_nlri_sanity_check (peer, afi, stream_pnt (s), nlri_len);
      if (ret < 0) 
        {
          zlog_info ("%s: (%s) NLRI doesn't pass sanity check",
                     __func__, peer->host);
	  return BGP_ATTR_PARSE_ERROR;
	}
    }

  mp_update->afi = afi;
  mp_update->safi = safi;
  mp_update->nlri = stream_pnt (s);
  mp_update->length = nlri_len;

  stream_forward_getp (s, nlri_len);

  return BGP_ATTR_PARSE_PROCEED;
#undef LEN_LEFT
}