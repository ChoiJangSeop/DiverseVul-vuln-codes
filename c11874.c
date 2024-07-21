bgp_show_peer (struct vty *vty, struct peer *p)
{
  struct bgp *bgp;
  char buf1[BUFSIZ];
  char timebuf[BGP_UPTIME_LEN];
  afi_t afi;
  safi_t safi;

  bgp = p->bgp;

  /* Configured IP address. */
  vty_out (vty, "BGP neighbor is %s, ", p->host);
  vty_out (vty, "remote AS %d, ", p->as);
  vty_out (vty, "local AS %d%s, ",
	   p->change_local_as ? p->change_local_as : p->local_as,
	   CHECK_FLAG (p->flags, PEER_FLAG_LOCAL_AS_NO_PREPEND) ?
	   " no-prepend" : "");
  vty_out (vty, "%s link%s",
	   p->as == p->local_as ? "internal" : "external",
	   VTY_NEWLINE);

  /* Description. */
  if (p->desc)
    vty_out (vty, " Description: %s%s", p->desc, VTY_NEWLINE);
  
  /* Peer-group */
  if (p->group)
    vty_out (vty, " Member of peer-group %s for session parameters%s",
	     p->group->name, VTY_NEWLINE);

  /* Administrative shutdown. */
  if (CHECK_FLAG (p->flags, PEER_FLAG_SHUTDOWN))
    vty_out (vty, " Administratively shut down%s", VTY_NEWLINE);

  /* BGP Version. */
  vty_out (vty, "  BGP version 4");
  vty_out (vty, ", remote router ID %s%s", 
	   inet_ntop (AF_INET, &p->remote_id, buf1, BUFSIZ),
	   VTY_NEWLINE);

  /* Confederation */
  if (CHECK_FLAG (bgp->config, BGP_CONFIG_CONFEDERATION)
      && bgp_confederation_peers_check (bgp, p->as))
    vty_out (vty, "  Neighbor under common administration%s", VTY_NEWLINE);
  
  /* Status. */
  vty_out (vty, "  BGP state = %s",  
	   LOOKUP (bgp_status_msg, p->status));
  if (p->status == Established) 
    vty_out (vty, ", up for %8s", 
	     peer_uptime (p->uptime, timebuf, BGP_UPTIME_LEN));
  else if (p->status == Active)
    {
      if (CHECK_FLAG (p->flags, PEER_FLAG_PASSIVE))
	vty_out (vty, " (passive)"); 
      else if (CHECK_FLAG (p->sflags, PEER_STATUS_NSF_WAIT))
	vty_out (vty, " (NSF passive)"); 
    }
  vty_out (vty, "%s", VTY_NEWLINE);
  
  /* read timer */
  vty_out (vty, "  Last read %s", peer_uptime (p->readtime, timebuf, BGP_UPTIME_LEN));

  /* Configured timer values. */
  vty_out (vty, ", hold time is %d, keepalive interval is %d seconds%s",
	   p->v_holdtime, p->v_keepalive, VTY_NEWLINE);
  if (CHECK_FLAG (p->config, PEER_CONFIG_TIMER))
    {
      vty_out (vty, "  Configured hold time is %d", p->holdtime);
      vty_out (vty, ", keepalive interval is %d seconds%s",
	       p->keepalive, VTY_NEWLINE);
    }

  /* Capability. */
  if (p->status == Established) 
    {
      if (p->cap
	  || p->afc_adv[AFI_IP][SAFI_UNICAST]
	  || p->afc_recv[AFI_IP][SAFI_UNICAST]
	  || p->afc_adv[AFI_IP][SAFI_MULTICAST]
	  || p->afc_recv[AFI_IP][SAFI_MULTICAST]
#ifdef HAVE_IPV6
	  || p->afc_adv[AFI_IP6][SAFI_UNICAST]
	  || p->afc_recv[AFI_IP6][SAFI_UNICAST]
	  || p->afc_adv[AFI_IP6][SAFI_MULTICAST]
	  || p->afc_recv[AFI_IP6][SAFI_MULTICAST]
#endif /* HAVE_IPV6 */
	  || p->afc_adv[AFI_IP][SAFI_MPLS_VPN]
	  || p->afc_recv[AFI_IP][SAFI_MPLS_VPN])
	{
	  vty_out (vty, "  Neighbor capabilities:%s", VTY_NEWLINE);

	  /* Dynamic */
	  if (CHECK_FLAG (p->cap, PEER_CAP_DYNAMIC_RCV)
	      || CHECK_FLAG (p->cap, PEER_CAP_DYNAMIC_ADV))
	    {
	      vty_out (vty, "    Dynamic:");
	      if (CHECK_FLAG (p->cap, PEER_CAP_DYNAMIC_ADV))
		vty_out (vty, " advertised");
	      if (CHECK_FLAG (p->cap, PEER_CAP_DYNAMIC_RCV))
		vty_out (vty, " %sreceived",
			 CHECK_FLAG (p->cap, PEER_CAP_DYNAMIC_ADV) ? "and " : "");
	      vty_out (vty, "%s", VTY_NEWLINE);
	    }

	  /* Route Refresh */
	  if (CHECK_FLAG (p->cap, PEER_CAP_REFRESH_ADV)
	      || CHECK_FLAG (p->cap, PEER_CAP_REFRESH_NEW_RCV)
	      || CHECK_FLAG (p->cap, PEER_CAP_REFRESH_OLD_RCV))
	    {
	      vty_out (vty, "    Route refresh:");
 	      if (CHECK_FLAG (p->cap, PEER_CAP_REFRESH_ADV))
		vty_out (vty, " advertised");
	      if (CHECK_FLAG (p->cap, PEER_CAP_REFRESH_NEW_RCV)
		  || CHECK_FLAG (p->cap, PEER_CAP_REFRESH_OLD_RCV))
		vty_out (vty, " %sreceived(%s)",
			 CHECK_FLAG (p->cap, PEER_CAP_REFRESH_ADV) ? "and " : "",
			 (CHECK_FLAG (p->cap, PEER_CAP_REFRESH_OLD_RCV)
			  && CHECK_FLAG (p->cap, PEER_CAP_REFRESH_NEW_RCV)) ?
			 "old & new" : CHECK_FLAG (p->cap, PEER_CAP_REFRESH_OLD_RCV) ? "old" : "new");

	      vty_out (vty, "%s", VTY_NEWLINE);
	    }

	  /* Multiprotocol Extensions */
	  for (afi = AFI_IP ; afi < AFI_MAX ; afi++)
	    for (safi = SAFI_UNICAST ; safi < SAFI_MAX ; safi++)
	      if (p->afc_adv[afi][safi] || p->afc_recv[afi][safi])
		{
		  vty_out (vty, "    Address family %s:", afi_safi_print (afi, safi));
		  if (p->afc_adv[afi][safi]) 
		    vty_out (vty, " advertised");
		  if (p->afc_recv[afi][safi])
		    vty_out (vty, " %sreceived", p->afc_adv[afi][safi] ? "and " : "");
		  vty_out (vty, "%s", VTY_NEWLINE);
		} 

	  /* Gracefull Restart */
	  if (CHECK_FLAG (p->cap, PEER_CAP_RESTART_RCV)
	      || CHECK_FLAG (p->cap, PEER_CAP_RESTART_ADV))
	    {
	      vty_out (vty, "    Graceful Restart Capabilty:");
	      if (CHECK_FLAG (p->cap, PEER_CAP_RESTART_ADV))
		vty_out (vty, " advertised");
	      if (CHECK_FLAG (p->cap, PEER_CAP_RESTART_RCV))
		vty_out (vty, " %sreceived",
			 CHECK_FLAG (p->cap, PEER_CAP_RESTART_ADV) ? "and " : "");
	      vty_out (vty, "%s", VTY_NEWLINE);

	      if (CHECK_FLAG (p->cap, PEER_CAP_RESTART_RCV))
		{
		  int restart_af_count = 0;

		  vty_out (vty, "      Remote Restart timer is %d seconds%s",
			   p->v_gr_restart, VTY_NEWLINE);	
		  vty_out (vty, "      Address families by peer:%s        ", VTY_NEWLINE);

		  for (afi = AFI_IP ; afi < AFI_MAX ; afi++)
		    for (safi = SAFI_UNICAST ; safi < SAFI_MAX ; safi++)
		      if (CHECK_FLAG (p->af_cap[afi][safi], PEER_CAP_RESTART_AF_RCV))
			{
			  vty_out (vty, "%s%s(%s)", restart_af_count ? ", " : "",
				   afi_safi_print (afi, safi),
				   CHECK_FLAG (p->af_cap[afi][safi], PEER_CAP_RESTART_AF_PRESERVE_RCV) ?
				   "preserved" : "not preserved");
			  restart_af_count++;
			}
		  if (! restart_af_count)
		    vty_out (vty, "none");
		  vty_out (vty, "%s", VTY_NEWLINE);
		}
	    }
	}
    }

  /* graceful restart information */
  if (CHECK_FLAG (p->cap, PEER_CAP_RESTART_RCV)
      || p->t_gr_restart
      || p->t_gr_stale)
    {
      int eor_send_af_count = 0;
      int eor_receive_af_count = 0;

      vty_out (vty, "  Graceful restart informations:%s", VTY_NEWLINE);
      if (p->status == Established) 
	{
	  vty_out (vty, "    End-of-RIB send: ");
	  for (afi = AFI_IP ; afi < AFI_MAX ; afi++)
	    for (safi = SAFI_UNICAST ; safi < SAFI_MAX ; safi++)
	      if (CHECK_FLAG (p->af_sflags[afi][safi], PEER_STATUS_EOR_SEND))
		{
		  vty_out (vty, "%s%s", eor_send_af_count ? ", " : "",
			   afi_safi_print (afi, safi));
		  eor_send_af_count++;
		}
	  vty_out (vty, "%s", VTY_NEWLINE);

	  vty_out (vty, "    End-of-RIB received: ");
	  for (afi = AFI_IP ; afi < AFI_MAX ; afi++)
	    for (safi = SAFI_UNICAST ; safi < SAFI_MAX ; safi++)
	      if (CHECK_FLAG (p->af_sflags[afi][safi], PEER_STATUS_EOR_RECEIVED))
		{
		  vty_out (vty, "%s%s", eor_receive_af_count ? ", " : "",
			   afi_safi_print (afi, safi));
		  eor_receive_af_count++;
		}
	  vty_out (vty, "%s", VTY_NEWLINE);
	}

      if (p->t_gr_restart)
        {
	  vty_out (vty, "    The remaining time of restart timer is %ld%s",
		   thread_timer_remain_second (p->t_gr_restart), VTY_NEWLINE);
	}
      if (p->t_gr_stale)
	{
	  vty_out (vty, "    The remaining time of stalepath timer is %ld%s",
		   thread_timer_remain_second (p->t_gr_stale), VTY_NEWLINE);
	}
    }

  /* Packet counts. */
  vty_out (vty, "  Message statistics:%s", VTY_NEWLINE);
  vty_out (vty, "    Inq depth is 0%s", VTY_NEWLINE);
  vty_out (vty, "    Outq depth is %ld%s", p->obuf->count, VTY_NEWLINE);
  vty_out (vty, "                         Sent       Rcvd%s", VTY_NEWLINE);
  vty_out (vty, "    Opens:         %10d %10d%s", p->open_out, p->open_in, VTY_NEWLINE);
  vty_out (vty, "    Notifications: %10d %10d%s", p->notify_out, p->notify_in, VTY_NEWLINE);
  vty_out (vty, "    Updates:       %10d %10d%s", p->update_out, p->update_in, VTY_NEWLINE);
  vty_out (vty, "    Keepalives:    %10d %10d%s", p->keepalive_out, p->keepalive_in, VTY_NEWLINE);
  vty_out (vty, "    Route Refresh: %10d %10d%s", p->refresh_out, p->refresh_in, VTY_NEWLINE);
  vty_out (vty, "    Capability:    %10d %10d%s", p->dynamic_cap_out, p->dynamic_cap_in, VTY_NEWLINE);
  vty_out (vty, "    Total:         %10d %10d%s", p->open_out + p->notify_out +
	   p->update_out + p->keepalive_out + p->refresh_out + p->dynamic_cap_out,
	   p->open_in + p->notify_in + p->update_in + p->keepalive_in + p->refresh_in +
	   p->dynamic_cap_in, VTY_NEWLINE);

  /* advertisement-interval */
  vty_out (vty, "  Minimum time between advertisement runs is %d seconds%s",
	   p->v_routeadv, VTY_NEWLINE);

  /* Update-source. */
  if (p->update_if || p->update_source)
    {
      vty_out (vty, "  Update source is ");
      if (p->update_if)
	vty_out (vty, "%s", p->update_if);
      else if (p->update_source)
	vty_out (vty, "%s",
		 sockunion2str (p->update_source, buf1, SU_ADDRSTRLEN));
      vty_out (vty, "%s", VTY_NEWLINE);
    }

  /* Default weight */
  if (CHECK_FLAG (p->config, PEER_CONFIG_WEIGHT))
    vty_out (vty, "  Default weight %d%s", p->weight,
	     VTY_NEWLINE);

  vty_out (vty, "%s", VTY_NEWLINE);

  /* Address Family Information */
  for (afi = AFI_IP ; afi < AFI_MAX ; afi++)
    for (safi = SAFI_UNICAST ; safi < SAFI_MAX ; safi++)
      if (p->afc[afi][safi])
	bgp_show_peer_afi (vty, p, afi, safi);

  vty_out (vty, "  Connections established %d; dropped %d%s",
	   p->established, p->dropped,
	   VTY_NEWLINE);

  if (! p->dropped)
    vty_out (vty, "  Last reset never%s", VTY_NEWLINE);
  else
    vty_out (vty, "  Last reset %s, due to %s%s",
            peer_uptime (p->resettime, timebuf, BGP_UPTIME_LEN),
            peer_down_str[(int) p->last_reset], VTY_NEWLINE);

  if (CHECK_FLAG (p->sflags, PEER_STATUS_PREFIX_OVERFLOW))
    {
      vty_out (vty, "  Peer had exceeded the max. no. of prefixes configured.%s", VTY_NEWLINE);

      if (p->t_pmax_restart)
	vty_out (vty, "  Reduce the no. of prefix from %s, will restart in %ld seconds%s",
		 p->host, thread_timer_remain_second (p->t_pmax_restart),
		 VTY_NEWLINE);
      else
	vty_out (vty, "  Reduce the no. of prefix and clear ip bgp %s to restore peering%s",
		 p->host, VTY_NEWLINE);
    }

  /* EBGP Multihop */
  if (peer_sort (p) != BGP_PEER_IBGP && p->ttl > 1)
    vty_out (vty, "  External BGP neighbor may be up to %d hops away.%s",
	     p->ttl, VTY_NEWLINE);

  /* Local address. */
  if (p->su_local)
    {
      vty_out (vty, "Local host: %s, Local port: %d%s",
	       sockunion2str (p->su_local, buf1, SU_ADDRSTRLEN),
	       ntohs (p->su_local->sin.sin_port),
	       VTY_NEWLINE);
    }
      
  /* Remote address. */
  if (p->su_remote)
    {
      vty_out (vty, "Foreign host: %s, Foreign port: %d%s",
	       sockunion2str (p->su_remote, buf1, SU_ADDRSTRLEN),
	       ntohs (p->su_remote->sin.sin_port),
	       VTY_NEWLINE);
    }

  /* Nexthop display. */
  if (p->su_local)
    {
      vty_out (vty, "Nexthop: %s%s", 
	       inet_ntop (AF_INET, &p->nexthop.v4, buf1, BUFSIZ),
	       VTY_NEWLINE);
#ifdef HAVE_IPV6
      vty_out (vty, "Nexthop global: %s%s", 
	       inet_ntop (AF_INET6, &p->nexthop.v6_global, buf1, BUFSIZ),
	       VTY_NEWLINE);
      vty_out (vty, "Nexthop local: %s%s",
	       inet_ntop (AF_INET6, &p->nexthop.v6_local, buf1, BUFSIZ),
	       VTY_NEWLINE);
      vty_out (vty, "BGP connection: %s%s",
	       p->shared_network ? "shared network" : "non shared network",
	       VTY_NEWLINE);
#endif /* HAVE_IPV6 */
    }

  /* Timer information. */
  if (p->t_start)
    vty_out (vty, "Next start timer due in %ld seconds%s",
	     thread_timer_remain_second (p->t_start), VTY_NEWLINE);
  if (p->t_connect)
    vty_out (vty, "Next connect timer due in %ld seconds%s",
	     thread_timer_remain_second (p->t_connect), VTY_NEWLINE);
  
  vty_out (vty, "Read thread: %s  Write thread: %s%s", 
	   p->t_read ? "on" : "off",
	   p->t_write ? "on" : "off",
	   VTY_NEWLINE);

  if (p->notify.code == BGP_NOTIFY_OPEN_ERR
      && p->notify.subcode == BGP_NOTIFY_OPEN_UNSUP_CAPBL)
    bgp_capability_vty_out (vty, p);
 
  vty_out (vty, "%s", VTY_NEWLINE);
}