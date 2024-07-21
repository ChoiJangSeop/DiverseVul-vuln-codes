static void sctp_do_8_2_transport_strike(sctp_cmd_seq_t *commands,
					 struct sctp_association *asoc,
					 struct sctp_transport *transport,
					 int is_hb)
{
	/* The check for association's overall error counter exceeding the
	 * threshold is done in the state function.
	 */
	/* We are here due to a timer expiration.  If the timer was
	 * not a HEARTBEAT, then normal error tracking is done.
	 * If the timer was a heartbeat, we only increment error counts
	 * when we already have an outstanding HEARTBEAT that has not
	 * been acknowledged.
	 * Additionally, some tranport states inhibit error increments.
	 */
	if (!is_hb) {
		asoc->overall_error_count++;
		if (transport->state != SCTP_INACTIVE)
			transport->error_count++;
	 } else if (transport->hb_sent) {
		if (transport->state != SCTP_UNCONFIRMED)
			asoc->overall_error_count++;
		if (transport->state != SCTP_INACTIVE)
			transport->error_count++;
	}

	/* If the transport error count is greater than the pf_retrans
	 * threshold, and less than pathmaxrtx, then mark this transport
	 * as Partially Failed, ee SCTP Quick Failover Draft, secon 5.1,
	 * point 1
	 */
	if ((transport->state != SCTP_PF) &&
	   (asoc->pf_retrans < transport->pathmaxrxt) &&
	   (transport->error_count > asoc->pf_retrans)) {

		sctp_assoc_control_transport(asoc, transport,
					     SCTP_TRANSPORT_PF,
					     0);

		/* Update the hb timer to resend a heartbeat every rto */
		sctp_cmd_hb_timer_update(commands, transport);
	}

	if (transport->state != SCTP_INACTIVE &&
	    (transport->error_count > transport->pathmaxrxt)) {
		SCTP_DEBUG_PRINTK_IPADDR("transport_strike:association %p",
					 " transport IP: port:%d failed.\n",
					 asoc,
					 (&transport->ipaddr),
					 ntohs(transport->ipaddr.v4.sin_port));
		sctp_assoc_control_transport(asoc, transport,
					     SCTP_TRANSPORT_DOWN,
					     SCTP_FAILED_THRESHOLD);
	}

	/* E2) For the destination address for which the timer
	 * expires, set RTO <- RTO * 2 ("back off the timer").  The
	 * maximum value discussed in rule C7 above (RTO.max) may be
	 * used to provide an upper bound to this doubling operation.
	 *
	 * Special Case:  the first HB doesn't trigger exponential backoff.
	 * The first unacknowledged HB triggers it.  We do this with a flag
	 * that indicates that we have an outstanding HB.
	 */
	if (!is_hb || transport->hb_sent) {
		transport->rto = min((transport->rto * 2), transport->asoc->rto_max);
	}
}