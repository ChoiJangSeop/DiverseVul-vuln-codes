bgp_capability_parse (struct peer *peer, u_char *pnt, u_char length,
		      u_char **error)
{
  int ret;
  u_char *end;
  struct capability cap;

  end = pnt + length;

  while (pnt < end)
    {
      afi_t afi;
      safi_t safi;

      /* Fetch structure to the byte stream. */
      memcpy (&cap, pnt, sizeof (struct capability));

      afi = ntohs(cap.mpc.afi);
      safi = cap.mpc.safi;

      if (BGP_DEBUG (normal, NORMAL))
	zlog_debug ("%s OPEN has CAPABILITY code: %d, length %d",
		   peer->host, cap.code, cap.length);

      /* We need at least capability code and capability length. */
      if (pnt + 2 > end)
	{
	  zlog_info ("%s Capability length error", peer->host);
	  bgp_notify_send (peer, BGP_NOTIFY_CEASE, 0);
	  return -1;
	}

      /* Capability length check. */
      if (pnt + (cap.length + 2) > end)
	{
	  zlog_info ("%s Capability length error", peer->host);
	  bgp_notify_send (peer, BGP_NOTIFY_CEASE, 0);
	  return -1;
	}

      /* We know MP Capability Code. */
      if (cap.code == CAPABILITY_CODE_MP)
	{
	  if (BGP_DEBUG (normal, NORMAL))
	    zlog_debug ("%s OPEN has MP_EXT CAP for afi/safi: %u/%u",
		       peer->host, afi, safi);

	  /* Ignore capability when override-capability is set. */
	  if (! CHECK_FLAG (peer->flags, PEER_FLAG_OVERRIDE_CAPABILITY))
	    {
	      /* Set negotiated value. */
	      ret = bgp_capability_mp (peer, &cap);

	      /* Unsupported Capability. */
	      if (ret < 0)
		{
		  /* Store return data. */
		  memcpy (*error, &cap, cap.length + 2);
		  *error += cap.length + 2;
		}
	    }
	}
      else if (cap.code == CAPABILITY_CODE_REFRESH
	       || cap.code == CAPABILITY_CODE_REFRESH_OLD)
	{
	  /* Check length. */
	  if (cap.length != CAPABILITY_CODE_REFRESH_LEN)
	    {
	      zlog_info ("%s Route Refresh Capability length error %d",
			 peer->host, cap.length);
	      bgp_notify_send (peer, BGP_NOTIFY_CEASE, 0);
	      return -1;
	    }

	  if (BGP_DEBUG (normal, NORMAL))
	    zlog_debug ("%s OPEN has ROUTE-REFRESH capability(%s) for all address-families",
		       peer->host,
		       cap.code == CAPABILITY_CODE_REFRESH_OLD ? "old" : "new");

	  /* BGP refresh capability */
	  if (cap.code == CAPABILITY_CODE_REFRESH_OLD)
	    SET_FLAG (peer->cap, PEER_CAP_REFRESH_OLD_RCV);
	  else
	    SET_FLAG (peer->cap, PEER_CAP_REFRESH_NEW_RCV);
	}
      else if (cap.code == CAPABILITY_CODE_ORF
	       || cap.code == CAPABILITY_CODE_ORF_OLD)
	bgp_capability_orf (peer, &cap, pnt + sizeof (struct capability));
      else if (cap.code == CAPABILITY_CODE_RESTART)
       {
         struct graceful_restart_af graf;
         u_int16_t restart_flag_time;
         int restart_bit = 0;
         u_char *restart_pnt;
         u_char *restart_end;

         /* Check length. */
         if (cap.length < CAPABILITY_CODE_RESTART_LEN)
           {
             zlog_info ("%s Graceful Restart Capability length error %d",
                        peer->host, cap.length);
             bgp_notify_send (peer, BGP_NOTIFY_CEASE, 0);
             return -1;
           }

         SET_FLAG (peer->cap, PEER_CAP_RESTART_RCV);
         restart_flag_time = ntohs(cap.mpc.afi);
         if (CHECK_FLAG (restart_flag_time, RESTART_R_BIT))
           restart_bit = 1;
         UNSET_FLAG (restart_flag_time, 0xF000);
	 peer->v_gr_restart = restart_flag_time;

         if (BGP_DEBUG (normal, NORMAL))
           {
             zlog_debug ("%s OPEN has Graceful Restart capability", peer->host);
             zlog_debug ("%s Peer has%srestarted. Restart Time : %d",
                        peer->host, restart_bit ? " " : " not ",
			peer->v_gr_restart);
           }

         restart_pnt = pnt + 4;
         restart_end = pnt + cap.length + 2;

         while (restart_pnt < restart_end)
           {
             memcpy (&graf, restart_pnt, sizeof (struct graceful_restart_af));

             afi = ntohs(graf.afi);
             safi = graf.safi;

             if (CHECK_FLAG (graf.flag, RESTART_F_BIT))
		SET_FLAG (peer->af_cap[afi][safi], PEER_CAP_RESTART_AF_PRESERVE_RCV);

             if (strcmp (afi_safi_print (afi, safi), "Unknown") == 0)
               {
                  if (BGP_DEBUG (normal, NORMAL))
                    zlog_debug ("%s Addr-family %d/%d(afi/safi) not supported. I gnore the Graceful Restart capability",
                               peer->host, afi, safi);
               }
             else if (! peer->afc[afi][safi])
               {
                  if (BGP_DEBUG (normal, NORMAL))
                     zlog_debug ("%s Addr-family %d/%d(afi/safi) not enabled. Ignore the Graceful Restart capability",
                                peer->host, afi, safi);
               }
             else
               {
                 if (BGP_DEBUG (normal, NORMAL))
                   zlog_debug ("%s Address family %s is%spreserved", peer->host,
			       afi_safi_print (afi, safi),
			       CHECK_FLAG (peer->af_cap[afi][safi],
			       PEER_CAP_RESTART_AF_PRESERVE_RCV)
			       ? " " : " not ");

                   SET_FLAG (peer->af_cap[afi][safi], PEER_CAP_RESTART_AF_RCV);
               }
             restart_pnt += 4;
           }
       }
      else if (cap.code == CAPABILITY_CODE_DYNAMIC)
	{
	  /* Check length. */
	  if (cap.length != CAPABILITY_CODE_DYNAMIC_LEN)
	    {
	      zlog_info ("%s Dynamic Capability length error %d",
			 peer->host, cap.length);
	      bgp_notify_send (peer, BGP_NOTIFY_CEASE, 0);
	      return -1;
	    }

	  if (BGP_DEBUG (normal, NORMAL))
	    zlog_debug ("%s OPEN has DYNAMIC capability", peer->host);

	  SET_FLAG (peer->cap, PEER_CAP_DYNAMIC_RCV);
	}
 
      else if (cap.code > 128)
	{
	  /* We don't send Notification for unknown vendor specific
	     capabilities.  It seems reasonable for now...  */
	  zlog_warn ("%s Vendor specific capability %d",
		     peer->host, cap.code);
	}
      else
	{
	  zlog_warn ("%s unrecognized capability code: %d - ignored",
		     peer->host, cap.code);
	  memcpy (*error, &cap, cap.length + 2);
	  *error += cap.length + 2;
	}

      pnt += cap.length + 2;
    }
  return 0;
}