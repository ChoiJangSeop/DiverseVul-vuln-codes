static int sctp_outq_flush_rtx(struct sctp_outq *q, struct sctp_packet *pkt,
			       int rtx_timeout, int *start_timer)
{
	struct list_head *lqueue;
	struct sctp_transport *transport = pkt->transport;
	sctp_xmit_t status;
	struct sctp_chunk *chunk, *chunk1;
	int fast_rtx;
	int error = 0;
	int timer = 0;
	int done = 0;

	lqueue = &q->retransmit;
	fast_rtx = q->fast_rtx;

	/* This loop handles time-out retransmissions, fast retransmissions,
	 * and retransmissions due to opening of whindow.
	 *
	 * RFC 2960 6.3.3 Handle T3-rtx Expiration
	 *
	 * E3) Determine how many of the earliest (i.e., lowest TSN)
	 * outstanding DATA chunks for the address for which the
	 * T3-rtx has expired will fit into a single packet, subject
	 * to the MTU constraint for the path corresponding to the
	 * destination transport address to which the retransmission
	 * is being sent (this may be different from the address for
	 * which the timer expires [see Section 6.4]). Call this value
	 * K. Bundle and retransmit those K DATA chunks in a single
	 * packet to the destination endpoint.
	 *
	 * [Just to be painfully clear, if we are retransmitting
	 * because a timeout just happened, we should send only ONE
	 * packet of retransmitted data.]
	 *
	 * For fast retransmissions we also send only ONE packet.  However,
	 * if we are just flushing the queue due to open window, we'll
	 * try to send as much as possible.
	 */
	list_for_each_entry_safe(chunk, chunk1, lqueue, transmitted_list) {
		/* If the chunk is abandoned, move it to abandoned list. */
		if (sctp_chunk_abandoned(chunk)) {
			list_del_init(&chunk->transmitted_list);
			sctp_insert_list(&q->abandoned,
					 &chunk->transmitted_list);
			continue;
		}

		/* Make sure that Gap Acked TSNs are not retransmitted.  A
		 * simple approach is just to move such TSNs out of the
		 * way and into a 'transmitted' queue and skip to the
		 * next chunk.
		 */
		if (chunk->tsn_gap_acked) {
			list_move_tail(&chunk->transmitted_list,
				       &transport->transmitted);
			continue;
		}

		/* If we are doing fast retransmit, ignore non-fast_rtransmit
		 * chunks
		 */
		if (fast_rtx && !chunk->fast_retransmit)
			continue;

redo:
		/* Attempt to append this chunk to the packet. */
		status = sctp_packet_append_chunk(pkt, chunk);

		switch (status) {
		case SCTP_XMIT_PMTU_FULL:
			if (!pkt->has_data && !pkt->has_cookie_echo) {
				/* If this packet did not contain DATA then
				 * retransmission did not happen, so do it
				 * again.  We'll ignore the error here since
				 * control chunks are already freed so there
				 * is nothing we can do.
				 */
				sctp_packet_transmit(pkt);
				goto redo;
			}

			/* Send this packet.  */
			error = sctp_packet_transmit(pkt);

			/* If we are retransmitting, we should only
			 * send a single packet.
			 * Otherwise, try appending this chunk again.
			 */
			if (rtx_timeout || fast_rtx)
				done = 1;
			else
				goto redo;

			/* Bundle next chunk in the next round.  */
			break;

		case SCTP_XMIT_RWND_FULL:
			/* Send this packet. */
			error = sctp_packet_transmit(pkt);

			/* Stop sending DATA as there is no more room
			 * at the receiver.
			 */
			done = 1;
			break;

		case SCTP_XMIT_NAGLE_DELAY:
			/* Send this packet. */
			error = sctp_packet_transmit(pkt);

			/* Stop sending DATA because of nagle delay. */
			done = 1;
			break;

		default:
			/* The append was successful, so add this chunk to
			 * the transmitted list.
			 */
			list_move_tail(&chunk->transmitted_list,
				       &transport->transmitted);

			/* Mark the chunk as ineligible for fast retransmit
			 * after it is retransmitted.
			 */
			if (chunk->fast_retransmit == SCTP_NEED_FRTX)
				chunk->fast_retransmit = SCTP_DONT_FRTX;

			q->empty = 0;
			break;
		}

		/* Set the timer if there were no errors */
		if (!error && !timer)
			timer = 1;

		if (done)
			break;
	}

	/* If we are here due to a retransmit timeout or a fast
	 * retransmit and if there are any chunks left in the retransmit
	 * queue that could not fit in the PMTU sized packet, they need
	 * to be marked as ineligible for a subsequent fast retransmit.
	 */
	if (rtx_timeout || fast_rtx) {
		list_for_each_entry(chunk1, lqueue, transmitted_list) {
			if (chunk1->fast_retransmit == SCTP_NEED_FRTX)
				chunk1->fast_retransmit = SCTP_DONT_FRTX;
		}
	}

	*start_timer = timer;

	/* Clear fast retransmit hint */
	if (fast_rtx)
		q->fast_rtx = 0;

	return error;
}