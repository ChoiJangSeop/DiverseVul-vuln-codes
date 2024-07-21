bgp_open_receive (struct peer *peer, bgp_size_t size)
{
  int ret;
  u_char version;
  u_char optlen;
  u_int16_t holdtime;
  u_int16_t send_holdtime;
  as_t remote_as;
  as_t as4 = 0;
  struct peer *realpeer;
  struct in_addr remote_id;
  int capability;
  u_int8_t notify_data_remote_as[2];
  u_int8_t notify_data_remote_id[4];

  realpeer = NULL;
  
  /* Parse open packet. */
  version = stream_getc (peer->ibuf);
  memcpy (notify_data_remote_as, stream_pnt (peer->ibuf), 2);
  remote_as  = stream_getw (peer->ibuf);
  holdtime = stream_getw (peer->ibuf);
  memcpy (notify_data_remote_id, stream_pnt (peer->ibuf), 4);
  remote_id.s_addr = stream_get_ipv4 (peer->ibuf);

  /* Receive OPEN message log  */
  if (BGP_DEBUG (normal, NORMAL))
    zlog_debug ("%s rcv OPEN, version %d, remote-as (in open) %u,"
                " holdtime %d, id %s",
	        peer->host, version, remote_as, holdtime,
	        inet_ntoa (remote_id));
  
  /* BEGIN to read the capability here, but dont do it yet */
  capability = 0;
  optlen = stream_getc (peer->ibuf);
  
  if (optlen != 0)
    {
      /* We need the as4 capability value *right now* because
       * if it is there, we have not got the remote_as yet, and without
       * that we do not know which peer is connecting to us now.
       */ 
      as4 = peek_for_as4_capability (peer, optlen);
    }
  
  /* Just in case we have a silly peer who sends AS4 capability set to 0 */
  if (CHECK_FLAG (peer->cap, PEER_CAP_AS4_RCV) && !as4)
    {
      zlog_err ("%s bad OPEN, got AS4 capability, but AS4 set to 0",
                peer->host);
      bgp_notify_send (peer, BGP_NOTIFY_OPEN_ERR,
                       BGP_NOTIFY_OPEN_BAD_PEER_AS);
      return -1;
    }
  
  if (remote_as == BGP_AS_TRANS)
    {
	  /* Take the AS4 from the capability.  We must have received the
	   * capability now!  Otherwise we have a asn16 peer who uses
	   * BGP_AS_TRANS, for some unknown reason.
	   */
      if (as4 == BGP_AS_TRANS)
        {
          zlog_err ("%s [AS4] NEW speaker using AS_TRANS for AS4, not allowed",
                    peer->host);
          bgp_notify_send (peer, BGP_NOTIFY_OPEN_ERR,
                 BGP_NOTIFY_OPEN_BAD_PEER_AS);
          return -1;
        }
      
      if (!as4 && BGP_DEBUG (as4, AS4))
        zlog_debug ("%s [AS4] OPEN remote_as is AS_TRANS, but no AS4."
                    " Odd, but proceeding.", peer->host);
      else if (as4 < BGP_AS_MAX && BGP_DEBUG (as4, AS4))
        zlog_debug ("%s [AS4] OPEN remote_as is AS_TRANS, but AS4 (%u) fits "
                    "in 2-bytes, very odd peer.", peer->host, as4);
      if (as4)
        remote_as = as4;
    } 
  else 
    {
      /* We may have a partner with AS4 who has an asno < BGP_AS_MAX */
      /* If we have got the capability, peer->as4cap must match remote_as */
      if (CHECK_FLAG (peer->cap, PEER_CAP_AS4_RCV)
          && as4 != remote_as)
        {
	  /* raise error, log this, close session */
	  zlog_err ("%s bad OPEN, got AS4 capability, but remote_as %u"
	            " mismatch with 16bit 'myasn' %u in open",
	            peer->host, as4, remote_as);
	  bgp_notify_send (peer, BGP_NOTIFY_OPEN_ERR,
			   BGP_NOTIFY_OPEN_BAD_PEER_AS);
	  return -1;
	}
    }

  /* Lookup peer from Open packet. */
  if (CHECK_FLAG (peer->sflags, PEER_STATUS_ACCEPT_PEER))
    {
      int as = 0;

      realpeer = peer_lookup_with_open (&peer->su, remote_as, &remote_id, &as);

      if (! realpeer)
	{
	  /* Peer's source IP address is check in bgp_accept(), so this
	     must be AS number mismatch or remote-id configuration
	     mismatch. */
	  if (as)
	    {
	      if (BGP_DEBUG (normal, NORMAL))
		zlog_debug ("%s bad OPEN, wrong router identifier %s",
			    peer->host, inet_ntoa (remote_id));
	      bgp_notify_send_with_data (peer, BGP_NOTIFY_OPEN_ERR, 
					 BGP_NOTIFY_OPEN_BAD_BGP_IDENT,
					 notify_data_remote_id, 4);
	    }
	  else
	    {
	      if (BGP_DEBUG (normal, NORMAL))
		zlog_debug ("%s bad OPEN, remote AS is %u, expected %u",
			    peer->host, remote_as, peer->as);
	      bgp_notify_send_with_data (peer, BGP_NOTIFY_OPEN_ERR,
					 BGP_NOTIFY_OPEN_BAD_PEER_AS,
					 notify_data_remote_as, 2);
	    }
	  return -1;
	}
    }

  /* When collision is detected and this peer is closed.  Retrun
     immidiately. */
  ret = bgp_collision_detect (peer, remote_id);
  if (ret < 0)
    return ret;

  /* Hack part. */
  if (CHECK_FLAG (peer->sflags, PEER_STATUS_ACCEPT_PEER))
    {
	if (realpeer->status == Established
	    && CHECK_FLAG (realpeer->sflags, PEER_STATUS_NSF_MODE))
	{
	  realpeer->last_reset = PEER_DOWN_NSF_CLOSE_SESSION;
	  SET_FLAG (realpeer->sflags, PEER_STATUS_NSF_WAIT);
	}
	else if (ret == 0 && realpeer->status != Active
	         && realpeer->status != OpenSent
		 && realpeer->status != OpenConfirm
		 && realpeer->status != Connect)
 	{
 	  /* XXX: This is an awful problem.. 
 	   *
 	   * According to the RFC we should just let this connection (of the
 	   * accepted 'peer') continue on to Established if the other
 	   * connection (the 'realpeer' one) is in state Connect, and deal
 	   * with the more larval FSM as/when it gets far enough to receive
 	   * an Open. We don't do that though, we instead close the (more
 	   * developed) accepted connection.
 	   *
 	   * This means there's a race, which if hit, can loop:
 	   *
 	   *       FSM for A                        FSM for B
 	   *  realpeer     accept-peer       realpeer     accept-peer
 	   *
 	   *  Connect                        Connect
 	   *               Active
 	   *               OpenSent          OpenSent
 	   *               <arrive here,
 	   *               Notify, delete>   
 	   *                                 Idle         Active
 	   *   OpenSent                                   OpenSent
 	   *                                              <arrive here,
 	   *                                              Notify, delete>
 	   *   Idle
 	   *   <wait>                        <wait>
 	   *   Connect                       Connect
 	   *
           *
 	   * If both sides are Quagga, they're almost certain to wait for
 	   * the same amount of time of course (which doesn't preclude other
 	   * implementations also waiting for same time). The race is
 	   * exacerbated by high-latency (in bgpd and/or the network).
 	   *
 	   * The reason we do this is because our FSM is tied to our peer
 	   * structure, which carries our configuration information, etc. 
 	   * I.e. we can't let the accepted-peer FSM continue on as it is,
 	   * cause it's not associated with any actual peer configuration -
 	   * it's just a dummy.
 	   *
 	   * It's possible we could hack-fix this by just bgp_stop'ing the
 	   * realpeer and continueing on with the 'transfer FSM' below. 
 	   * Ideally, we need to seperate FSMs from struct peer.
 	   *
 	   * Setting one side to passive avoids the race, as a workaround.
 	   */
 	  if (BGP_DEBUG (events, EVENTS))
	    zlog_debug ("%s peer status is %s close connection",
			realpeer->host, LOOKUP (bgp_status_msg,
			realpeer->status));
	  bgp_notify_send (peer, BGP_NOTIFY_CEASE,
			   BGP_NOTIFY_CEASE_CONNECT_REJECT);

 	  return -1;
 	}

      if (BGP_DEBUG (events, EVENTS))
	zlog_debug ("%s [Event] Transfer accept BGP peer to real (state %s)",
		   peer->host, 
		   LOOKUP (bgp_status_msg, realpeer->status));

      bgp_stop (realpeer);
      
      /* Transfer file descriptor. */
      realpeer->fd = peer->fd;
      peer->fd = -1;

      /* Transfer input buffer. */
      stream_free (realpeer->ibuf);
      realpeer->ibuf = peer->ibuf;
      realpeer->packet_size = peer->packet_size;
      peer->ibuf = NULL;

      /* Transfer status. */
      realpeer->status = peer->status;
      bgp_stop (peer);
      
      /* peer pointer change. Open packet send to neighbor. */
      peer = realpeer;
      bgp_open_send (peer);
      if (peer->fd < 0)
	{
	  zlog_err ("bgp_open_receive peer's fd is negative value %d",
		    peer->fd);
	  return -1;
	}
      BGP_READ_ON (peer->t_read, bgp_read, peer->fd);
    }

  /* remote router-id check. */
  if (remote_id.s_addr == 0
      || IPV4_CLASS_DE (ntohl (remote_id.s_addr))
      || ntohl (peer->local_id.s_addr) == ntohl (remote_id.s_addr))
    {
      if (BGP_DEBUG (normal, NORMAL))
	zlog_debug ("%s bad OPEN, wrong router identifier %s",
		   peer->host, inet_ntoa (remote_id));
      bgp_notify_send_with_data (peer, 
				 BGP_NOTIFY_OPEN_ERR, 
				 BGP_NOTIFY_OPEN_BAD_BGP_IDENT,
				 notify_data_remote_id, 4);
      return -1;
    }

  /* Set remote router-id */
  peer->remote_id = remote_id;

  /* Peer BGP version check. */
  if (version != BGP_VERSION_4)
    {
      u_int8_t maxver = BGP_VERSION_4;
      if (BGP_DEBUG (normal, NORMAL))
	zlog_debug ("%s bad protocol version, remote requested %d, local request %d",
		   peer->host, version, BGP_VERSION_4);
      bgp_notify_send_with_data (peer, 
				 BGP_NOTIFY_OPEN_ERR, 
				 BGP_NOTIFY_OPEN_UNSUP_VERSION,
				 &maxver, 1);
      return -1;
    }

  /* Check neighbor as number. */
  if (remote_as != peer->as)
    {
      if (BGP_DEBUG (normal, NORMAL))
	zlog_debug ("%s bad OPEN, remote AS is %u, expected %u",
		   peer->host, remote_as, peer->as);
      bgp_notify_send_with_data (peer, 
				 BGP_NOTIFY_OPEN_ERR, 
				 BGP_NOTIFY_OPEN_BAD_PEER_AS,
				 notify_data_remote_as, 2);
      return -1;
    }

  /* From the rfc: Upon receipt of an OPEN message, a BGP speaker MUST
     calculate the value of the Hold Timer by using the smaller of its
     configured Hold Time and the Hold Time received in the OPEN message.
     The Hold Time MUST be either zero or at least three seconds.  An
     implementation may reject connections on the basis of the Hold Time. */

  if (holdtime < 3 && holdtime != 0)
    {
      bgp_notify_send (peer,
		       BGP_NOTIFY_OPEN_ERR, 
		       BGP_NOTIFY_OPEN_UNACEP_HOLDTIME);
      return -1;
    }
    
  /* From the rfc: A reasonable maximum time between KEEPALIVE messages
     would be one third of the Hold Time interval.  KEEPALIVE messages
     MUST NOT be sent more frequently than one per second.  An
     implementation MAY adjust the rate at which it sends KEEPALIVE
     messages as a function of the Hold Time interval. */

  if (CHECK_FLAG (peer->config, PEER_CONFIG_TIMER))
    send_holdtime = peer->holdtime;
  else
    send_holdtime = peer->bgp->default_holdtime;

  if (holdtime < send_holdtime)
    peer->v_holdtime = holdtime;
  else
    peer->v_holdtime = send_holdtime;

  peer->v_keepalive = peer->v_holdtime / 3;

  /* Open option part parse. */
  if (optlen != 0) 
    {
      ret = bgp_open_option_parse (peer, optlen, &capability);
      if (ret < 0)
	return ret;
    }
  else
    {
      if (BGP_DEBUG (normal, NORMAL))
	zlog_debug ("%s rcvd OPEN w/ OPTION parameter len: 0",
		   peer->host);
    }

  /* Override capability. */
  if (! capability || CHECK_FLAG (peer->flags, PEER_FLAG_OVERRIDE_CAPABILITY))
    {
      peer->afc_nego[AFI_IP][SAFI_UNICAST] = peer->afc[AFI_IP][SAFI_UNICAST];
      peer->afc_nego[AFI_IP][SAFI_MULTICAST] = peer->afc[AFI_IP][SAFI_MULTICAST];
      peer->afc_nego[AFI_IP6][SAFI_UNICAST] = peer->afc[AFI_IP6][SAFI_UNICAST];
      peer->afc_nego[AFI_IP6][SAFI_MULTICAST] = peer->afc[AFI_IP6][SAFI_MULTICAST];
    }

  /* Get sockname. */
  bgp_getsockname (peer);

  BGP_EVENT_ADD (peer, Receive_OPEN_message);

  peer->packet_size = 0;
  if (peer->ibuf)
    stream_reset (peer->ibuf);

  return 0;
}