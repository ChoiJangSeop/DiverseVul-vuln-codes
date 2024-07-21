static void sctp_assoc_bh_rcv(struct work_struct *work)
{
	struct sctp_association *asoc =
		container_of(work, struct sctp_association,
			     base.inqueue.immediate);
	struct net *net = sock_net(asoc->base.sk);
	struct sctp_endpoint *ep;
	struct sctp_chunk *chunk;
	struct sctp_inq *inqueue;
	int state;
	sctp_subtype_t subtype;
	int error = 0;

	/* The association should be held so we should be safe. */
	ep = asoc->ep;

	inqueue = &asoc->base.inqueue;
	sctp_association_hold(asoc);
	while (NULL != (chunk = sctp_inq_pop(inqueue))) {
		state = asoc->state;
		subtype = SCTP_ST_CHUNK(chunk->chunk_hdr->type);

		/* SCTP-AUTH, Section 6.3:
		 *    The receiver has a list of chunk types which it expects
		 *    to be received only after an AUTH-chunk.  This list has
		 *    been sent to the peer during the association setup.  It
		 *    MUST silently discard these chunks if they are not placed
		 *    after an AUTH chunk in the packet.
		 */
		if (sctp_auth_recv_cid(subtype.chunk, asoc) && !chunk->auth)
			continue;

		/* Remember where the last DATA chunk came from so we
		 * know where to send the SACK.
		 */
		if (sctp_chunk_is_data(chunk))
			asoc->peer.last_data_from = chunk->transport;
		else
			SCTP_INC_STATS(net, SCTP_MIB_INCTRLCHUNKS);

		if (chunk->transport)
			chunk->transport->last_time_heard = jiffies;

		/* Run through the state machine. */
		error = sctp_do_sm(net, SCTP_EVENT_T_CHUNK, subtype,
				   state, ep, asoc, chunk, GFP_ATOMIC);

		/* Check to see if the association is freed in response to
		 * the incoming chunk.  If so, get out of the while loop.
		 */
		if (asoc->base.dead)
			break;

		/* If there is an error on chunk, discard this packet. */
		if (error && chunk)
			chunk->pdiscard = 1;
	}
	sctp_association_put(asoc);
}