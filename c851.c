bgp_capability_orf_entry (struct peer *peer, struct capability_header *hdr)
{
  struct stream *s = BGP_INPUT (peer);
  struct capability_orf_entry entry;
  afi_t afi;
  safi_t safi;
  u_char type;
  u_char mode;
  u_int16_t sm_cap = 0; /* capability send-mode receive */
  u_int16_t rm_cap = 0; /* capability receive-mode receive */ 
  int i;

  /* ORF Entry header */
  bgp_capability_mp_data (s, &entry.mpc);
  entry.num = stream_getc (s);
  afi = entry.mpc.afi;
  safi = entry.mpc.safi;
  
  if (BGP_DEBUG (normal, NORMAL))
    zlog_debug ("%s ORF Cap entry for afi/safi: %u/%u",
	        peer->host, entry.mpc.afi, entry.mpc.safi);

  /* Check AFI and SAFI. */
  if (!bgp_afi_safi_valid_indices (entry.mpc.afi, &safi))
    {
      zlog_info ("%s Addr-family %d/%d not supported."
                 " Ignoring the ORF capability",
                 peer->host, entry.mpc.afi, entry.mpc.safi);
      return 0;
    }
  
  /* validate number field */
  if (sizeof (struct capability_orf_entry) + (entry.num * 2) > hdr->length)
    {
      zlog_info ("%s ORF Capability entry length error,"
                 " Cap length %u, num %u",
                 peer->host, hdr->length, entry.num);
      bgp_notify_send (peer, BGP_NOTIFY_CEASE, 0);
      return -1;
    }

  for (i = 0 ; i < entry.num ; i++)
    {
      type = stream_getc(s);
      mode = stream_getc(s);
      
      /* ORF Mode error check */
      switch (mode)
        {
          case ORF_MODE_BOTH:
          case ORF_MODE_SEND:
          case ORF_MODE_RECEIVE:
            break;
          default:
	    bgp_capability_orf_not_support (peer, afi, safi, type, mode);
	    continue;
	}
      /* ORF Type and afi/safi error checks */
      /* capcode versus type */
      switch (hdr->code)
        {
          case CAPABILITY_CODE_ORF:
            switch (type)
              {
                case ORF_TYPE_PREFIX:
                  break;
                default:
                  bgp_capability_orf_not_support (peer, afi, safi, type, mode);
                  continue;
              }
            break;
          case CAPABILITY_CODE_ORF_OLD:
            switch (type)
              {
                case ORF_TYPE_PREFIX_OLD:
                  break;
                default:
                  bgp_capability_orf_not_support (peer, afi, safi, type, mode);
                  continue;
              }
            break;
          default:
            bgp_capability_orf_not_support (peer, afi, safi, type, mode);
            continue;
        }
                
      /* AFI vs SAFI */
      if (!((afi == AFI_IP && safi == SAFI_UNICAST)
            || (afi == AFI_IP && safi == SAFI_MULTICAST)
            || (afi == AFI_IP6 && safi == SAFI_UNICAST)))
        {
          bgp_capability_orf_not_support (peer, afi, safi, type, mode);
          continue;
        }
      
      if (BGP_DEBUG (normal, NORMAL))
        zlog_debug ("%s OPEN has %s ORF capability"
                    " as %s for afi/safi: %d/%d",
                    peer->host, LOOKUP (orf_type_str, type),
                    LOOKUP (orf_mode_str, mode),
                    entry.mpc.afi, safi);

      if (hdr->code == CAPABILITY_CODE_ORF)
	{
          sm_cap = PEER_CAP_ORF_PREFIX_SM_RCV;
          rm_cap = PEER_CAP_ORF_PREFIX_RM_RCV;
	}
      else if (hdr->code == CAPABILITY_CODE_ORF_OLD)
	{
          sm_cap = PEER_CAP_ORF_PREFIX_SM_OLD_RCV;
          rm_cap = PEER_CAP_ORF_PREFIX_RM_OLD_RCV;
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