recv_and_process_client_pkt(void /*int fd*/)
{
	ssize_t          size;
	//uint8_t          version;
	len_and_sockaddr *to;
	struct sockaddr  *from;
	msg_t            msg;
	uint8_t          query_status;
	l_fixedpt_t      query_xmttime;

	to = get_sock_lsa(G_listen_fd);
	from = xzalloc(to->len);

	size = recv_from_to(G_listen_fd, &msg, sizeof(msg), MSG_DONTWAIT, from, &to->u.sa, to->len);
	if (size != NTP_MSGSIZE_NOAUTH && size != NTP_MSGSIZE) {
		char *addr;
		if (size < 0) {
			if (errno == EAGAIN)
				goto bail;
			bb_perror_msg_and_die("recv");
		}
		addr = xmalloc_sockaddr2dotted_noport(from);
		bb_error_msg("malformed packet received from %s: size %u", addr, (int)size);
		free(addr);
		goto bail;
	}

	query_status = msg.m_status;
	query_xmttime = msg.m_xmttime;

	/* Build a reply packet */
	memset(&msg, 0, sizeof(msg));
	msg.m_status = G.stratum < MAXSTRAT ? (G.ntp_status & LI_MASK) : LI_ALARM;
	msg.m_status |= (query_status & VERSION_MASK);
	msg.m_status |= ((query_status & MODE_MASK) == MODE_CLIENT) ?
			MODE_SERVER : MODE_SYM_PAS;
	msg.m_stratum = G.stratum;
	msg.m_ppoll = G.poll_exp;
	msg.m_precision_exp = G_precision_exp;
	/* this time was obtained between poll() and recv() */
	msg.m_rectime = d_to_lfp(G.cur_time);
	msg.m_xmttime = d_to_lfp(gettime1900d()); /* this instant */
	if (G.peer_cnt == 0) {
		/* we have no peers: "stratum 1 server" mode. reftime = our own time */
		G.reftime = G.cur_time;
	}
	msg.m_reftime = d_to_lfp(G.reftime);
	msg.m_orgtime = query_xmttime;
	msg.m_rootdelay = d_to_sfp(G.rootdelay);
//simple code does not do this, fix simple code!
	msg.m_rootdisp = d_to_sfp(G.rootdisp);
	//version = (query_status & VERSION_MASK); /* ... >> VERSION_SHIFT - done below instead */
	msg.m_refid = G.refid; // (version > (3 << VERSION_SHIFT)) ? G.refid : G.refid3;

	/* We reply from the local address packet was sent to,
	 * this makes to/from look swapped here: */
	do_sendto(G_listen_fd,
		/*from:*/ &to->u.sa, /*to:*/ from, /*addrlen:*/ to->len,
		&msg, size);

 bail:
	free(to);
	free(from);
}