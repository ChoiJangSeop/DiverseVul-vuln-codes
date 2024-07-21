void tipc_mon_rcv(struct net *net, void *data, u16 dlen, u32 addr,
		  struct tipc_mon_state *state, int bearer_id)
{
	struct tipc_monitor *mon = tipc_monitor(net, bearer_id);
	struct tipc_mon_domain *arrv_dom = data;
	struct tipc_mon_domain dom_bef;
	struct tipc_mon_domain *dom;
	struct tipc_peer *peer;
	u16 new_member_cnt = mon_le16_to_cpu(arrv_dom->member_cnt);
	int new_dlen = dom_rec_len(arrv_dom, new_member_cnt);
	u16 new_gen = mon_le16_to_cpu(arrv_dom->gen);
	u16 acked_gen = mon_le16_to_cpu(arrv_dom->ack_gen);
	u16 arrv_dlen = mon_le16_to_cpu(arrv_dom->len);
	bool probing = state->probing;
	int i, applied_bef;

	state->probing = false;

	/* Sanity check received domain record */
	if (dlen < dom_rec_len(arrv_dom, 0))
		return;
	if (dlen != dom_rec_len(arrv_dom, new_member_cnt))
		return;
	if (dlen < new_dlen || arrv_dlen != new_dlen)
		return;

	/* Synch generation numbers with peer if link just came up */
	if (!state->synched) {
		state->peer_gen = new_gen - 1;
		state->acked_gen = acked_gen;
		state->synched = true;
	}

	if (more(acked_gen, state->acked_gen))
		state->acked_gen = acked_gen;

	/* Drop duplicate unless we are waiting for a probe response */
	if (!more(new_gen, state->peer_gen) && !probing)
		return;

	write_lock_bh(&mon->lock);
	peer = get_peer(mon, addr);
	if (!peer || !peer->is_up)
		goto exit;

	/* Peer is confirmed, stop any ongoing probing */
	peer->down_cnt = 0;

	/* Task is done for duplicate record */
	if (!more(new_gen, state->peer_gen))
		goto exit;

	state->peer_gen = new_gen;

	/* Cache current domain record for later use */
	dom_bef.member_cnt = 0;
	dom = peer->domain;
	if (dom)
		memcpy(&dom_bef, dom, dom->len);

	/* Transform and store received domain record */
	if (!dom || (dom->len < new_dlen)) {
		kfree(dom);
		dom = kmalloc(new_dlen, GFP_ATOMIC);
		peer->domain = dom;
		if (!dom)
			goto exit;
	}
	dom->len = new_dlen;
	dom->gen = new_gen;
	dom->member_cnt = new_member_cnt;
	dom->up_map = mon_le64_to_cpu(arrv_dom->up_map);
	for (i = 0; i < new_member_cnt; i++)
		dom->members[i] = mon_le32_to_cpu(arrv_dom->members[i]);

	/* Update peers affected by this domain record */
	applied_bef = peer->applied;
	mon_apply_domain(mon, peer);
	mon_identify_lost_members(peer, &dom_bef, applied_bef);
	mon_assign_roles(mon, peer_head(peer));
exit:
	write_unlock_bh(&mon->lock);
}