void sctp_transport_update_rto(struct sctp_transport *tp, __u32 rtt)
{
	/* Check for valid transport.  */
	SCTP_ASSERT(tp, "NULL transport", return);

	/* We should not be doing any RTO updates unless rto_pending is set.  */
	SCTP_ASSERT(tp->rto_pending, "rto_pending not set", return);

	if (tp->rttvar || tp->srtt) {
		struct net *net = sock_net(tp->asoc->base.sk);
		/* 6.3.1 C3) When a new RTT measurement R' is made, set
		 * RTTVAR <- (1 - RTO.Beta) * RTTVAR + RTO.Beta * |SRTT - R'|
		 * SRTT <- (1 - RTO.Alpha) * SRTT + RTO.Alpha * R'
		 */

		/* Note:  The above algorithm has been rewritten to
		 * express rto_beta and rto_alpha as inverse powers
		 * of two.
		 * For example, assuming the default value of RTO.Alpha of
		 * 1/8, rto_alpha would be expressed as 3.
		 */
		tp->rttvar = tp->rttvar - (tp->rttvar >> net->sctp.rto_beta)
			+ (((__u32)abs64((__s64)tp->srtt - (__s64)rtt)) >> net->sctp.rto_beta);
		tp->srtt = tp->srtt - (tp->srtt >> net->sctp.rto_alpha)
			+ (rtt >> net->sctp.rto_alpha);
	} else {
		/* 6.3.1 C2) When the first RTT measurement R is made, set
		 * SRTT <- R, RTTVAR <- R/2.
		 */
		tp->srtt = rtt;
		tp->rttvar = rtt >> 1;
	}

	/* 6.3.1 G1) Whenever RTTVAR is computed, if RTTVAR = 0, then
	 * adjust RTTVAR <- G, where G is the CLOCK GRANULARITY.
	 */
	if (tp->rttvar == 0)
		tp->rttvar = SCTP_CLOCK_GRANULARITY;

	/* 6.3.1 C3) After the computation, update RTO <- SRTT + 4 * RTTVAR. */
	tp->rto = tp->srtt + (tp->rttvar << 2);

	/* 6.3.1 C6) Whenever RTO is computed, if it is less than RTO.Min
	 * seconds then it is rounded up to RTO.Min seconds.
	 */
	if (tp->rto < tp->asoc->rto_min)
		tp->rto = tp->asoc->rto_min;

	/* 6.3.1 C7) A maximum value may be placed on RTO provided it is
	 * at least RTO.max seconds.
	 */
	if (tp->rto > tp->asoc->rto_max)
		tp->rto = tp->asoc->rto_max;

	tp->rtt = rtt;

	/* Reset rto_pending so that a new RTT measurement is started when a
	 * new data chunk is sent.
	 */
	tp->rto_pending = 0;

	SCTP_DEBUG_PRINTK("%s: transport: %p, rtt: %d, srtt: %d "
			  "rttvar: %d, rto: %ld\n", __func__,
			  tp, rtt, tp->srtt, tp->rttvar, tp->rto);
}