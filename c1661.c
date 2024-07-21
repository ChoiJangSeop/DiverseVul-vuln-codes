bgp_mp_unreach_parse (struct peer *peer, const bgp_size_t length,
                      const u_char flag, u_char *startp,
		      struct bgp_nlri *mp_withdraw)
{
  struct stream *s;
  afi_t afi;
  safi_t safi;
  u_int16_t withdraw_len;
  int ret;
  bgp_size_t total;

  total = length + (CHECK_FLAG (flag, BGP_ATTR_FLAG_EXTLEN) ? 4 : 3);
  /* Flag checks. */
  if (bgp_attr_flag_invalid (peer, BGP_ATTR_MP_UNREACH_NLRI, flag))
    return bgp_attr_malformed (peer, BGP_ATTR_MP_UNREACH_NLRI, flag,
                               BGP_NOTIFY_UPDATE_ATTR_FLAG_ERR,
                               startp, total);

  s = peer->ibuf;
  
#define BGP_MP_UNREACH_MIN_SIZE 3
  if ((length > STREAM_READABLE(s)) || (length <  BGP_MP_UNREACH_MIN_SIZE))
    return BGP_ATTR_PARSE_ERROR;
  
  afi = stream_getw (s);
  safi = stream_getc (s);
  
  withdraw_len = length - BGP_MP_UNREACH_MIN_SIZE;

  if (safi != SAFI_MPLS_LABELED_VPN)
    {
      ret = bgp_nlri_sanity_check (peer, afi, stream_pnt (s), withdraw_len);
      if (ret < 0)
	return BGP_ATTR_PARSE_ERROR;
    }

  mp_withdraw->afi = afi;
  mp_withdraw->safi = safi;
  mp_withdraw->nlri = stream_pnt (s);
  mp_withdraw->length = withdraw_len;

  stream_forward_getp (s, withdraw_len);

  return BGP_ATTR_PARSE_PROCEED;
}