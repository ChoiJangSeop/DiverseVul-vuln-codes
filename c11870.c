bgp_open_option_parse (struct peer *peer, u_char length, int *capability)
{
  int ret;
  u_char *end;
  u_char opt_type;
  u_char opt_length;
  u_char *pnt;
  u_char *error;
  u_char error_data[BGP_MAX_PACKET_SIZE];

  /* Fetch pointer. */
  pnt = stream_pnt (peer->ibuf);

  ret = 0;
  opt_type = 0;
  opt_length = 0;
  end = pnt + length;
  error = error_data;

  if (BGP_DEBUG (normal, NORMAL))
    zlog_debug ("%s rcv OPEN w/ OPTION parameter len: %u",
	       peer->host, length);
  
  while (pnt < end) 
    {
      /* Check the length. */
      if (pnt + 2 > end)
	{
	  zlog_info ("%s Option length error", peer->host);
	  bgp_notify_send (peer, BGP_NOTIFY_CEASE, 0);
	  return -1;
	}

      /* Fetch option type and length. */
      opt_type = *pnt++;
      opt_length = *pnt++;
      
      /* Option length check. */
      if (pnt + opt_length > end)
	{
	  zlog_info ("%s Option length error", peer->host);
	  bgp_notify_send (peer, BGP_NOTIFY_CEASE, 0);
	  return -1;
	}

      if (BGP_DEBUG (normal, NORMAL))
	zlog_debug ("%s rcvd OPEN w/ optional parameter type %u (%s) len %u",
		   peer->host, opt_type,
		   opt_type == BGP_OPEN_OPT_AUTH ? "Authentication" :
		   opt_type == BGP_OPEN_OPT_CAP ? "Capability" : "Unknown",
		   opt_length);
  
      switch (opt_type)
	{
	case BGP_OPEN_OPT_AUTH:
	  ret = bgp_auth_parse (peer, pnt, opt_length);
	  break;
	case BGP_OPEN_OPT_CAP:
	  ret = bgp_capability_parse (peer, pnt, opt_length, &error);
	  *capability = 1;
	  break;
	default:
	  bgp_notify_send (peer, 
			   BGP_NOTIFY_OPEN_ERR, 
			   BGP_NOTIFY_OPEN_UNSUP_PARAM); 
	  ret = -1;
	  break;
	}

      /* Parse error.  To accumulate all unsupported capability codes,
         bgp_capability_parse does not return -1 when encounter
         unsupported capability code.  To detect that, please check
         error and erro_data pointer, like below.  */
      if (ret < 0)
	return -1;

      /* Forward pointer. */
      pnt += opt_length;
    }

  /* All OPEN option is parsed.  Check capability when strict compare
     flag is enabled.*/
  if (CHECK_FLAG (peer->flags, PEER_FLAG_STRICT_CAP_MATCH))
    {
      /* If Unsupported Capability exists. */
      if (error != error_data)
	{
	  bgp_notify_send_with_data (peer, 
				     BGP_NOTIFY_OPEN_ERR, 
				     BGP_NOTIFY_OPEN_UNSUP_CAPBL, 
				     error_data, error - error_data);
	  return -1;
	}

      /* Check local capability does not negotiated with remote
         peer. */
      if (! strict_capability_same (peer))
	{
	  bgp_notify_send (peer, 
			   BGP_NOTIFY_OPEN_ERR, 
			   BGP_NOTIFY_OPEN_UNSUP_CAPBL);
	  return -1;
	}
    }

  /* Check there is no common capability send Unsupported Capability
     error. */
  if (*capability && ! CHECK_FLAG (peer->flags, PEER_FLAG_OVERRIDE_CAPABILITY))
    {
      if (! peer->afc_nego[AFI_IP][SAFI_UNICAST] 
	  && ! peer->afc_nego[AFI_IP][SAFI_MULTICAST]
	  && ! peer->afc_nego[AFI_IP][SAFI_MPLS_VPN]
	  && ! peer->afc_nego[AFI_IP6][SAFI_UNICAST]
	  && ! peer->afc_nego[AFI_IP6][SAFI_MULTICAST])
	{
	  plog_err (peer->log, "%s [Error] No common capability", peer->host);

	  if (error != error_data)

	    bgp_notify_send_with_data (peer, 
				       BGP_NOTIFY_OPEN_ERR, 
				       BGP_NOTIFY_OPEN_UNSUP_CAPBL, 
				       error_data, error - error_data);
	  else
	    bgp_notify_send (peer, 
			     BGP_NOTIFY_OPEN_ERR, 
			     BGP_NOTIFY_OPEN_UNSUP_CAPBL);
	  return -1;
	}
    }
  return 0;
}