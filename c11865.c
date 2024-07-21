bgp_capability_vty_out (struct vty *vty, struct peer *peer)
{
  char *pnt;
  char *end;
  struct capability cap;

  pnt = peer->notify.data;
  end = pnt + peer->notify.length;

  while (pnt < end)
    {
      memcpy(&cap, pnt, sizeof(struct capability));

      if (pnt + 2 > end)
	return;
      if (pnt + (cap.length + 2) > end)
	return;

      if (cap.code == CAPABILITY_CODE_MP)
	{
	  vty_out (vty, "  Capability error for: Multi protocol ");

	  switch (ntohs (cap.mpc.afi))
	    {
	    case AFI_IP:
	      vty_out (vty, "AFI IPv4, ");
	      break;
	    case AFI_IP6:
	      vty_out (vty, "AFI IPv6, ");
	      break;
	    default:
	      vty_out (vty, "AFI Unknown %d, ", ntohs (cap.mpc.afi));
	      break;
	    }
	  switch (cap.mpc.safi)
	    {
	    case SAFI_UNICAST:
	      vty_out (vty, "SAFI Unicast");
	      break;
	    case SAFI_MULTICAST:
	      vty_out (vty, "SAFI Multicast");
	      break;
	    case SAFI_UNICAST_MULTICAST:
	      vty_out (vty, "SAFI Unicast Multicast");
	      break;
	    case BGP_SAFI_VPNV4:
	      vty_out (vty, "SAFI MPLS-VPN");
	      break;
	    default:
	      vty_out (vty, "SAFI Unknown %d ", cap.mpc.safi);
	      break;
	    }
	  vty_out (vty, "%s", VTY_NEWLINE);
	}
      else if (cap.code >= 128)
	vty_out (vty, "  Capability error: vendor specific capability code %d",
		 cap.code);
      else
	vty_out (vty, "  Capability error: unknown capability code %d", 
		 cap.code);

      pnt += cap.length + 2;
    }
}