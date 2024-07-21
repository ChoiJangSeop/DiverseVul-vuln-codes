bgp_capability_orf (struct peer *peer, struct capability *cap,
		    u_char *pnt)
{
  afi_t afi = ntohs(cap->mpc.afi);
  safi_t safi = cap->mpc.safi;
  u_char number_of_orfs;
  u_char type;
  u_char mode;
  u_int16_t sm_cap = 0; /* capability send-mode receive */
  u_int16_t rm_cap = 0; /* capability receive-mode receive */ 
  int i;

  /* Check length. */
  if (cap->length < 7)
    {
      zlog_info ("%s ORF Capability length error %d",
		 peer->host, cap->length);
		 bgp_notify_send (peer, BGP_NOTIFY_CEASE, 0);
      return -1;
    }

  if (BGP_DEBUG (normal, NORMAL))
    zlog_debug ("%s OPEN has ORF CAP(%s) for afi/safi: %u/%u",
	       peer->host, (cap->code == CAPABILITY_CODE_ORF ?
                       "new" : "old"), afi, safi);

  /* Check AFI and SAFI. */
  if ((afi != AFI_IP && afi != AFI_IP6)
      || (safi != SAFI_UNICAST && safi != SAFI_MULTICAST
	  && safi != BGP_SAFI_VPNV4))
    {
      zlog_info ("%s Addr-family %d/%d not supported. Ignoring the ORF capability",
                 peer->host, afi, safi);
      return -1;
    }

  number_of_orfs = *pnt++;

  for (i = 0 ; i < number_of_orfs ; i++)
    {
      type = *pnt++;
      mode = *pnt++;

      /* ORF Mode error check */
      if (mode != ORF_MODE_BOTH && mode != ORF_MODE_SEND
	  && mode != ORF_MODE_RECEIVE)
	{
	  bgp_capability_orf_not_support (peer, afi, safi, type, mode);
	  continue;
	}

      /* ORF Type and afi/safi error check */
      if (cap->code == CAPABILITY_CODE_ORF)
	{
	  if (type == ORF_TYPE_PREFIX &&
	      ((afi == AFI_IP && safi == SAFI_UNICAST)
		|| (afi == AFI_IP && safi == SAFI_MULTICAST)
		|| (afi == AFI_IP6 && safi == SAFI_UNICAST)))
	    {
	      sm_cap = PEER_CAP_ORF_PREFIX_SM_RCV;
	      rm_cap = PEER_CAP_ORF_PREFIX_RM_RCV;
	      if (BGP_DEBUG (normal, NORMAL))
		zlog_debug ("%s OPEN has Prefixlist ORF(%d) capability as %s for afi/safi: %d/%d",
			   peer->host, ORF_TYPE_PREFIX, (mode == ORF_MODE_SEND ? "SEND" :
			   mode == ORF_MODE_RECEIVE ? "RECEIVE" : "BOTH") , afi, safi);
	    }
	  else
	    {
	      bgp_capability_orf_not_support (peer, afi, safi, type, mode);
	      continue;
	    }
	}
      else if (cap->code == CAPABILITY_CODE_ORF_OLD)
	{
	  if (type == ORF_TYPE_PREFIX_OLD &&
	      ((afi == AFI_IP && safi == SAFI_UNICAST)
		|| (afi == AFI_IP && safi == SAFI_MULTICAST)
		|| (afi == AFI_IP6 && safi == SAFI_UNICAST)))
	    {
	      sm_cap = PEER_CAP_ORF_PREFIX_SM_OLD_RCV;
	      rm_cap = PEER_CAP_ORF_PREFIX_RM_OLD_RCV;
	      if (BGP_DEBUG (normal, NORMAL))
		zlog_debug ("%s OPEN has Prefixlist ORF(%d) capability as %s for afi/safi: %d/%d",
			   peer->host, ORF_TYPE_PREFIX_OLD, (mode == ORF_MODE_SEND ? "SEND" :
			   mode == ORF_MODE_RECEIVE ? "RECEIVE" : "BOTH") , afi, safi);
	    }
	  else
	    {
	      bgp_capability_orf_not_support (peer, afi, safi, type, mode);
	      continue;
	    }
	}
      else
	{
	  bgp_capability_orf_not_support (peer, afi, safi, type, mode);
	  continue;
	}

      switch (mode)
	{
	  case ORF_MODE_BOTH:
	    SET_FLAG (peer->af_cap[afi][safi], sm_cap);
	    SET_FLAG (peer->af_cap[afi][safi], rm_cap);
	    break;
	  case ORF_MODE_SEND:
	    SET_FLAG (peer->af_cap[afi][safi], sm_cap);
	    break;
	  case ORF_MODE_RECEIVE:
	    SET_FLAG (peer->af_cap[afi][safi], rm_cap);
	    break;
	}
    }
  return 0;
}