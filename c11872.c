bgp_capability_mp (struct peer *peer, struct capability *cap)
{
  if (ntohs (cap->mpc.afi) == AFI_IP)
    {
      if (cap->mpc.safi == SAFI_UNICAST)
	{
	  peer->afc_recv[AFI_IP][SAFI_UNICAST] = 1;

	  if (peer->afc[AFI_IP][SAFI_UNICAST])
	    peer->afc_nego[AFI_IP][SAFI_UNICAST] = 1;
	  else
	    return -1;
	}
      else if (cap->mpc.safi == SAFI_MULTICAST) 
	{
	  peer->afc_recv[AFI_IP][SAFI_MULTICAST] = 1;

	  if (peer->afc[AFI_IP][SAFI_MULTICAST])
	    peer->afc_nego[AFI_IP][SAFI_MULTICAST] = 1;
	  else
	    return -1;
	}
      else if (cap->mpc.safi == BGP_SAFI_VPNV4)
	{
	  peer->afc_recv[AFI_IP][SAFI_MPLS_VPN] = 1;

	  if (peer->afc[AFI_IP][SAFI_MPLS_VPN])
	    peer->afc_nego[AFI_IP][SAFI_MPLS_VPN] = 1;
	  else
	    return -1;
	}
      else
	return -1;
    }
#ifdef HAVE_IPV6
  else if (ntohs (cap->mpc.afi) == AFI_IP6)
    {
      if (cap->mpc.safi == SAFI_UNICAST)
	{
	  peer->afc_recv[AFI_IP6][SAFI_UNICAST] = 1;

	  if (peer->afc[AFI_IP6][SAFI_UNICAST])
	    peer->afc_nego[AFI_IP6][SAFI_UNICAST] = 1;
	  else
	    return -1;
	}
      else if (cap->mpc.safi == SAFI_MULTICAST)
	{
	  peer->afc_recv[AFI_IP6][SAFI_MULTICAST] = 1;

	  if (peer->afc[AFI_IP6][SAFI_MULTICAST])
	    peer->afc_nego[AFI_IP6][SAFI_MULTICAST] = 1;
	  else
	    return -1;
	}
      else
	return -1;
    }
#endif /* HAVE_IPV6 */
  else
    {
      /* Unknown Address Family. */
      return -1;
    }

  return 0;
}