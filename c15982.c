static int tipc_link_proto_rcv(struct tipc_link *l, struct sk_buff *skb,
			       struct sk_buff_head *xmitq)
{
	struct tipc_msg *hdr = buf_msg(skb);
	struct tipc_gap_ack_blks *ga = NULL;
	bool reply = msg_probe(hdr), retransmitted = false;
	u16 dlen = msg_data_sz(hdr), glen = 0;
	u16 peers_snd_nxt =  msg_next_sent(hdr);
	u16 peers_tol = msg_link_tolerance(hdr);
	u16 peers_prio = msg_linkprio(hdr);
	u16 gap = msg_seq_gap(hdr);
	u16 ack = msg_ack(hdr);
	u16 rcv_nxt = l->rcv_nxt;
	u16 rcvgap = 0;
	int mtyp = msg_type(hdr);
	int rc = 0, released;
	char *if_name;
	void *data;

	trace_tipc_proto_rcv(skb, false, l->name);
	if (tipc_link_is_blocked(l) || !xmitq)
		goto exit;

	if (tipc_own_addr(l->net) > msg_prevnode(hdr))
		l->net_plane = msg_net_plane(hdr);

	skb_linearize(skb);
	hdr = buf_msg(skb);
	data = msg_data(hdr);

	if (!tipc_link_validate_msg(l, hdr)) {
		trace_tipc_skb_dump(skb, false, "PROTO invalid (1)!");
		trace_tipc_link_dump(l, TIPC_DUMP_NONE, "PROTO invalid (1)!");
		goto exit;
	}

	switch (mtyp) {
	case RESET_MSG:
	case ACTIVATE_MSG:
		/* Complete own link name with peer's interface name */
		if_name =  strrchr(l->name, ':') + 1;
		if (sizeof(l->name) - (if_name - l->name) <= TIPC_MAX_IF_NAME)
			break;
		if (msg_data_sz(hdr) < TIPC_MAX_IF_NAME)
			break;
		strncpy(if_name, data, TIPC_MAX_IF_NAME);

		/* Update own tolerance if peer indicates a non-zero value */
		if (in_range(peers_tol, TIPC_MIN_LINK_TOL, TIPC_MAX_LINK_TOL)) {
			l->tolerance = peers_tol;
			l->bc_rcvlink->tolerance = peers_tol;
		}
		/* Update own priority if peer's priority is higher */
		if (in_range(peers_prio, l->priority + 1, TIPC_MAX_LINK_PRI))
			l->priority = peers_prio;

		/* If peer is going down we want full re-establish cycle */
		if (msg_peer_stopping(hdr)) {
			rc = tipc_link_fsm_evt(l, LINK_FAILURE_EVT);
			break;
		}

		/* If this endpoint was re-created while peer was ESTABLISHING
		 * it doesn't know current session number. Force re-synch.
		 */
		if (mtyp == ACTIVATE_MSG && msg_dest_session_valid(hdr) &&
		    l->session != msg_dest_session(hdr)) {
			if (less(l->session, msg_dest_session(hdr)))
				l->session = msg_dest_session(hdr) + 1;
			break;
		}

		/* ACTIVATE_MSG serves as PEER_RESET if link is already down */
		if (mtyp == RESET_MSG || !link_is_up(l))
			rc = tipc_link_fsm_evt(l, LINK_PEER_RESET_EVT);

		/* ACTIVATE_MSG takes up link if it was already locally reset */
		if (mtyp == ACTIVATE_MSG && l->state == LINK_ESTABLISHING)
			rc = TIPC_LINK_UP_EVT;

		l->peer_session = msg_session(hdr);
		l->in_session = true;
		l->peer_bearer_id = msg_bearer_id(hdr);
		if (l->mtu > msg_max_pkt(hdr))
			l->mtu = msg_max_pkt(hdr);
		break;

	case STATE_MSG:
		l->rcv_nxt_state = msg_seqno(hdr) + 1;

		/* Update own tolerance if peer indicates a non-zero value */
		if (in_range(peers_tol, TIPC_MIN_LINK_TOL, TIPC_MAX_LINK_TOL)) {
			l->tolerance = peers_tol;
			l->bc_rcvlink->tolerance = peers_tol;
		}
		/* Update own prio if peer indicates a different value */
		if ((peers_prio != l->priority) &&
		    in_range(peers_prio, 1, TIPC_MAX_LINK_PRI)) {
			l->priority = peers_prio;
			rc = tipc_link_fsm_evt(l, LINK_FAILURE_EVT);
		}

		l->silent_intv_cnt = 0;
		l->stats.recv_states++;
		if (msg_probe(hdr))
			l->stats.recv_probes++;

		if (!link_is_up(l)) {
			if (l->state == LINK_ESTABLISHING)
				rc = TIPC_LINK_UP_EVT;
			break;
		}

		/* Receive Gap ACK blocks from peer if any */
		glen = tipc_get_gap_ack_blks(&ga, l, hdr, true);

		tipc_mon_rcv(l->net, data + glen, dlen - glen, l->addr,
			     &l->mon_state, l->bearer_id);

		/* Send NACK if peer has sent pkts we haven't received yet */
		if ((reply || msg_is_keepalive(hdr)) &&
		    more(peers_snd_nxt, rcv_nxt) &&
		    !tipc_link_is_synching(l) &&
		    skb_queue_empty(&l->deferdq))
			rcvgap = peers_snd_nxt - l->rcv_nxt;
		if (rcvgap || reply)
			tipc_link_build_proto_msg(l, STATE_MSG, 0, reply,
						  rcvgap, 0, 0, xmitq);

		released = tipc_link_advance_transmq(l, l, ack, gap, ga, xmitq,
						     &retransmitted, &rc);
		if (gap)
			l->stats.recv_nacks++;
		if (released || retransmitted)
			tipc_link_update_cwin(l, released, retransmitted);
		if (released)
			tipc_link_advance_backlog(l, xmitq);
		if (unlikely(!skb_queue_empty(&l->wakeupq)))
			link_prepare_wakeup(l);
	}
exit:
	kfree_skb(skb);
	return rc;
}