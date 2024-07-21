bgp_attr_aspath_check (struct peer *peer, struct attr *attr, u_char flag)
{
  /* These checks were part of bgp_attr_aspath, but with
   * as4 we should to check aspath things when
   * aspath synthesizing with as4_path has already taken place.
   * Otherwise we check ASPATH and use the synthesized thing, and that is
   * not right.
   * So do the checks later, i.e. here
   */
  struct bgp *bgp = peer->bgp;
  struct aspath *aspath;

  bgp = peer->bgp;
    
  /* Confederation sanity check. */
  if ((peer_sort (peer) == BGP_PEER_CONFED && ! aspath_left_confed_check (attr->aspath)) ||
     (peer_sort (peer) == BGP_PEER_EBGP && aspath_confed_check (attr->aspath)))
    {
      zlog (peer->log, LOG_ERR, "Malformed AS path from %s", peer->host);
      return bgp_attr_malformed (peer, BGP_ATTR_AS_PATH, flag,
                                 BGP_NOTIFY_UPDATE_MAL_AS_PATH,
                                 NULL, 0);
    }

  /* First AS check for EBGP. */
  if (bgp != NULL && bgp_flag_check (bgp, BGP_FLAG_ENFORCE_FIRST_AS))
    {
      if (peer_sort (peer) == BGP_PEER_EBGP 
	  && ! aspath_firstas_check (attr->aspath, peer->as))
 	{
 	  zlog (peer->log, LOG_ERR,
 		"%s incorrect first AS (must be %u)", peer->host, peer->as);
          return bgp_attr_malformed (peer, BGP_ATTR_AS_PATH, flag,
                                     BGP_NOTIFY_UPDATE_MAL_AS_PATH,
                                     NULL, 0);
 	}
    }

  /* local-as prepend */
  if (peer->change_local_as &&
      ! CHECK_FLAG (peer->flags, PEER_FLAG_LOCAL_AS_NO_PREPEND))
    {
      aspath = aspath_dup (attr->aspath);
      aspath = aspath_add_seq (aspath, peer->change_local_as);
      aspath_unintern (&attr->aspath);
      attr->aspath = aspath_intern (aspath);
    }

  return BGP_ATTR_PARSE_PROCEED;
}