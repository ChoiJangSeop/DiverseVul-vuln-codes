struct sctp_transport *sctp_assoc_add_peer(struct sctp_association *asoc,
					   const union sctp_addr *addr,
					   const gfp_t gfp,
					   const int peer_state)
{
	struct net *net = sock_net(asoc->base.sk);
	struct sctp_transport *peer;
	struct sctp_sock *sp;
	unsigned short port;

	sp = sctp_sk(asoc->base.sk);

	/* AF_INET and AF_INET6 share common port field. */
	port = ntohs(addr->v4.sin_port);

	SCTP_DEBUG_PRINTK_IPADDR("sctp_assoc_add_peer:association %p addr: ",
				 " port: %d state:%d\n",
				 asoc,
				 addr,
				 port,
				 peer_state);

	/* Set the port if it has not been set yet.  */
	if (0 == asoc->peer.port)
		asoc->peer.port = port;

	/* Check to see if this is a duplicate. */
	peer = sctp_assoc_lookup_paddr(asoc, addr);
	if (peer) {
		/* An UNKNOWN state is only set on transports added by
		 * user in sctp_connectx() call.  Such transports should be
		 * considered CONFIRMED per RFC 4960, Section 5.4.
		 */
		if (peer->state == SCTP_UNKNOWN) {
			peer->state = SCTP_ACTIVE;
		}
		return peer;
	}

	peer = sctp_transport_new(net, addr, gfp);
	if (!peer)
		return NULL;

	sctp_transport_set_owner(peer, asoc);

	/* Initialize the peer's heartbeat interval based on the
	 * association configured value.
	 */
	peer->hbinterval = asoc->hbinterval;

	/* Set the path max_retrans.  */
	peer->pathmaxrxt = asoc->pathmaxrxt;

	/* And the partial failure retrnas threshold */
	peer->pf_retrans = asoc->pf_retrans;

	/* Initialize the peer's SACK delay timeout based on the
	 * association configured value.
	 */
	peer->sackdelay = asoc->sackdelay;
	peer->sackfreq = asoc->sackfreq;

	/* Enable/disable heartbeat, SACK delay, and path MTU discovery
	 * based on association setting.
	 */
	peer->param_flags = asoc->param_flags;

	sctp_transport_route(peer, NULL, sp);

	/* Initialize the pmtu of the transport. */
	if (peer->param_flags & SPP_PMTUD_DISABLE) {
		if (asoc->pathmtu)
			peer->pathmtu = asoc->pathmtu;
		else
			peer->pathmtu = SCTP_DEFAULT_MAXSEGMENT;
	}

	/* If this is the first transport addr on this association,
	 * initialize the association PMTU to the peer's PMTU.
	 * If not and the current association PMTU is higher than the new
	 * peer's PMTU, reset the association PMTU to the new peer's PMTU.
	 */
	if (asoc->pathmtu)
		asoc->pathmtu = min_t(int, peer->pathmtu, asoc->pathmtu);
	else
		asoc->pathmtu = peer->pathmtu;

	SCTP_DEBUG_PRINTK("sctp_assoc_add_peer:association %p PMTU set to "
			  "%d\n", asoc, asoc->pathmtu);
	peer->pmtu_pending = 0;

	asoc->frag_point = sctp_frag_point(asoc, asoc->pathmtu);

	/* The asoc->peer.port might not be meaningful yet, but
	 * initialize the packet structure anyway.
	 */
	sctp_packet_init(&peer->packet, peer, asoc->base.bind_addr.port,
			 asoc->peer.port);

	/* 7.2.1 Slow-Start
	 *
	 * o The initial cwnd before DATA transmission or after a sufficiently
	 *   long idle period MUST be set to
	 *      min(4*MTU, max(2*MTU, 4380 bytes))
	 *
	 * o The initial value of ssthresh MAY be arbitrarily high
	 *   (for example, implementations MAY use the size of the
	 *   receiver advertised window).
	 */
	peer->cwnd = min(4*asoc->pathmtu, max_t(__u32, 2*asoc->pathmtu, 4380));

	/* At this point, we may not have the receiver's advertised window,
	 * so initialize ssthresh to the default value and it will be set
	 * later when we process the INIT.
	 */
	peer->ssthresh = SCTP_DEFAULT_MAXWINDOW;

	peer->partial_bytes_acked = 0;
	peer->flight_size = 0;
	peer->burst_limited = 0;

	/* Set the transport's RTO.initial value */
	peer->rto = asoc->rto_initial;

	/* Set the peer's active state. */
	peer->state = peer_state;

	/* Attach the remote transport to our asoc.  */
	list_add_tail(&peer->transports, &asoc->peer.transport_addr_list);
	asoc->peer.transport_count++;

	/* If we do not yet have a primary path, set one.  */
	if (!asoc->peer.primary_path) {
		sctp_assoc_set_primary(asoc, peer);
		asoc->peer.retran_path = peer;
	}

	if (asoc->peer.active_path == asoc->peer.retran_path &&
	    peer->state != SCTP_UNCONFIRMED) {
		asoc->peer.retran_path = peer;
	}

	return peer;
}