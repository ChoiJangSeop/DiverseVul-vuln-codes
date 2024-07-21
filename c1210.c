void sctp_transport_reset(struct sctp_transport *t)
{
	struct sctp_association *asoc = t->asoc;

	/* RFC 2960 (bis), Section 5.2.4
	 * All the congestion control parameters (e.g., cwnd, ssthresh)
	 * related to this peer MUST be reset to their initial values
	 * (see Section 6.2.1)
	 */
	t->cwnd = min(4*asoc->pathmtu, max_t(__u32, 2*asoc->pathmtu, 4380));
	t->burst_limited = 0;
	t->ssthresh = asoc->peer.i.a_rwnd;
	t->rto = asoc->rto_initial;
	t->rtt = 0;
	t->srtt = 0;
	t->rttvar = 0;

	/* Reset these additional varibles so that we have a clean
	 * slate.
	 */
	t->partial_bytes_acked = 0;
	t->flight_size = 0;
	t->error_count = 0;
	t->rto_pending = 0;
	t->hb_sent = 0;

	/* Initialize the state information for SFR-CACC */
	t->cacc.changeover_active = 0;
	t->cacc.cycling_changeover = 0;
	t->cacc.next_tsn_at_change = 0;
	t->cacc.cacc_saw_newack = 0;
}