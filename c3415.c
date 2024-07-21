req_get_traps(
	sockaddr_u *srcadr,
	endpt *inter,
	struct req_pkt *inpkt
	)
{
	struct info_trap *it;
	struct ctl_trap *tr;
	size_t i;

	if (num_ctl_traps == 0) {
		req_ack(srcadr, inter, inpkt, INFO_ERR_NODATA);
		return;
	}

	it = (struct info_trap *)prepare_pkt(srcadr, inter, inpkt,
	    v6sizeof(struct info_trap));

	for (i = 0, tr = ctl_traps; i < COUNTOF(ctl_traps); i++, tr++) {
		if (tr->tr_flags & TRAP_INUSE) {
			if (IS_IPV4(&tr->tr_addr)) {
				if (tr->tr_localaddr == any_interface)
					it->local_address = 0;
				else
					it->local_address
					    = NSRCADR(&tr->tr_localaddr->sin);
				it->trap_address = NSRCADR(&tr->tr_addr);
				if (client_v6_capable)
					it->v6_flag = 0;
			} else {
				if (!client_v6_capable)
					continue;
				it->local_address6 
				    = SOCK_ADDR6(&tr->tr_localaddr->sin);
				it->trap_address6 = SOCK_ADDR6(&tr->tr_addr);
				it->v6_flag = 1;
			}
			it->trap_port = NSRCPORT(&tr->tr_addr);
			it->sequence = htons(tr->tr_sequence);
			it->settime = htonl((u_int32)(current_time - tr->tr_settime));
			it->origtime = htonl((u_int32)(current_time - tr->tr_origtime));
			it->resets = htonl((u_int32)tr->tr_resets);
			it->flags = htonl((u_int32)tr->tr_flags);
			it = (struct info_trap *)more_pkt();
		}
	}
	flush_pkt();
}