bgp_show_summary (struct vty *vty, struct bgp *bgp, int afi, int safi)
{
  struct peer *peer;
  struct listnode *node, *nnode;
  unsigned int count = 0;
  char timebuf[BGP_UPTIME_LEN];
  int len;

  /* Header string for each address family. */
  static char header[] = "Neighbor        V    AS MsgRcvd MsgSent   TblVer  InQ OutQ Up/Down  State/PfxRcd";
  
  for (ALL_LIST_ELEMENTS (bgp->peer, node, nnode, peer))
    {
      if (peer->afc[afi][safi])
	{
          if (!count)
            {
              unsigned long ents;
              char memstrbuf[MTYPE_MEMSTR_LEN];
              
              /* Usage summary and header */
              vty_out (vty,
                       "BGP router identifier %s, local AS number %d%s",
                       inet_ntoa (bgp->router_id), bgp->as, VTY_NEWLINE);

              ents = bgp_table_count (bgp->rib[afi][safi]);
              vty_out (vty, "RIB entries %ld, using %s of memory%s", ents,
                       mtype_memstr (memstrbuf, sizeof (memstrbuf),
                                     ents * sizeof (struct bgp_node)),
                       VTY_NEWLINE);
              
              /* Peer related usage */
              ents = listcount (bgp->peer);
              vty_out (vty, "Peers %ld, using %s of memory%s",
                       ents,
                       mtype_memstr (memstrbuf, sizeof (memstrbuf),
                                     ents * sizeof (struct peer)),
                       VTY_NEWLINE);
              
              if ((ents = listcount (bgp->rsclient)))
                vty_out (vty, "RS-Client peers %ld, using %s of memory%s",
                         ents,
                         mtype_memstr (memstrbuf, sizeof (memstrbuf),
                                       ents * sizeof (struct peer)),
                         VTY_NEWLINE);
              
              if ((ents = listcount (bgp->group)))
                vty_out (vty, "Peer groups %ld, using %s of memory%s", ents,
                         mtype_memstr (memstrbuf, sizeof (memstrbuf),
                                       ents * sizeof (struct peer_group)),
                         VTY_NEWLINE);

              if (CHECK_FLAG (bgp->af_flags[afi][safi], BGP_CONFIG_DAMPENING))
                vty_out (vty, "Dampening enabled.%s", VTY_NEWLINE);
              vty_out (vty, "%s", VTY_NEWLINE);
              vty_out (vty, "%s%s", header, VTY_NEWLINE);
            }
          
	  count++;

	  len = vty_out (vty, "%s", peer->host);
	  len = 16 - len;
	  if (len < 1)
	    vty_out (vty, "%s%*s", VTY_NEWLINE, 16, " ");
	  else
	    vty_out (vty, "%*s", len, " ");

	  vty_out (vty, "4 ");

	  vty_out (vty, "%5d %7d %7d %8d %4d %4ld ",
		   peer->as,
		   peer->open_in + peer->update_in + peer->keepalive_in
		   + peer->notify_in + peer->refresh_in + peer->dynamic_cap_in,
		   peer->open_out + peer->update_out + peer->keepalive_out
		   + peer->notify_out + peer->refresh_out
		   + peer->dynamic_cap_out,
		   0, 0, peer->obuf->count);

	  vty_out (vty, "%8s", 
		   peer_uptime (peer->uptime, timebuf, BGP_UPTIME_LEN));

	  if (peer->status == Established)
	    {
	      vty_out (vty, " %8ld", peer->pcount[afi][safi]);
	    }
	  else
	    {
	      if (CHECK_FLAG (peer->flags, PEER_FLAG_SHUTDOWN))
		vty_out (vty, " Idle (Admin)");
	      else if (CHECK_FLAG (peer->sflags, PEER_STATUS_PREFIX_OVERFLOW))
		vty_out (vty, " Idle (PfxCt)");
	      else
		vty_out (vty, " %-11s", LOOKUP(bgp_status_msg, peer->status));
	    }

	  vty_out (vty, "%s", VTY_NEWLINE);
	}
    }

  if (count)
    vty_out (vty, "%sTotal number of neighbors %d%s", VTY_NEWLINE,
	     count, VTY_NEWLINE);
  else
    vty_out (vty, "No %s neighbor is configured%s",
	     afi == AFI_IP ? "IPv4" : "IPv6", VTY_NEWLINE);
  return CMD_SUCCESS;
}