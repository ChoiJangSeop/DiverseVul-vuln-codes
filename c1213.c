static int sctp_outq_flush(struct sctp_outq *q, int rtx_timeout)
{
	struct sctp_packet *packet;
	struct sctp_packet singleton;
	struct sctp_association *asoc = q->asoc;
	__u16 sport = asoc->base.bind_addr.port;
	__u16 dport = asoc->peer.port;
	__u32 vtag = asoc->peer.i.init_tag;
	struct sctp_transport *transport = NULL;
	struct sctp_transport *new_transport;
	struct sctp_chunk *chunk, *tmp;
	sctp_xmit_t status;
	int error = 0;
	int start_timer = 0;
	int one_packet = 0;

	/* These transports have chunks to send. */
	struct list_head transport_list;
	struct list_head *ltransport;

	INIT_LIST_HEAD(&transport_list);
	packet = NULL;

	/*
	 * 6.10 Bundling
	 *   ...
	 *   When bundling control chunks with DATA chunks, an
	 *   endpoint MUST place control chunks first in the outbound
	 *   SCTP packet.  The transmitter MUST transmit DATA chunks
	 *   within a SCTP packet in increasing order of TSN.
	 *   ...
	 */

	list_for_each_entry_safe(chunk, tmp, &q->control_chunk_list, list) {
		/* RFC 5061, 5.3
		 * F1) This means that until such time as the ASCONF
		 * containing the add is acknowledged, the sender MUST
		 * NOT use the new IP address as a source for ANY SCTP
		 * packet except on carrying an ASCONF Chunk.
		 */
		if (asoc->src_out_of_asoc_ok &&
		    chunk->chunk_hdr->type != SCTP_CID_ASCONF)
			continue;

		list_del_init(&chunk->list);

		/* Pick the right transport to use. */
		new_transport = chunk->transport;

		if (!new_transport) {
			/*
			 * If we have a prior transport pointer, see if
			 * the destination address of the chunk
			 * matches the destination address of the
			 * current transport.  If not a match, then
			 * try to look up the transport with a given
			 * destination address.  We do this because
			 * after processing ASCONFs, we may have new
			 * transports created.
			 */
			if (transport &&
			    sctp_cmp_addr_exact(&chunk->dest,
						&transport->ipaddr))
					new_transport = transport;
			else
				new_transport = sctp_assoc_lookup_paddr(asoc,
								&chunk->dest);

			/* if we still don't have a new transport, then
			 * use the current active path.
			 */
			if (!new_transport)
				new_transport = asoc->peer.active_path;
		} else if ((new_transport->state == SCTP_INACTIVE) ||
			   (new_transport->state == SCTP_UNCONFIRMED) ||
			   (new_transport->state == SCTP_PF)) {
			/* If the chunk is Heartbeat or Heartbeat Ack,
			 * send it to chunk->transport, even if it's
			 * inactive.
			 *
			 * 3.3.6 Heartbeat Acknowledgement:
			 * ...
			 * A HEARTBEAT ACK is always sent to the source IP
			 * address of the IP datagram containing the
			 * HEARTBEAT chunk to which this ack is responding.
			 * ...
			 *
			 * ASCONF_ACKs also must be sent to the source.
			 */
			if (chunk->chunk_hdr->type != SCTP_CID_HEARTBEAT &&
			    chunk->chunk_hdr->type != SCTP_CID_HEARTBEAT_ACK &&
			    chunk->chunk_hdr->type != SCTP_CID_ASCONF_ACK)
				new_transport = asoc->peer.active_path;
		}

		/* Are we switching transports?
		 * Take care of transport locks.
		 */
		if (new_transport != transport) {
			transport = new_transport;
			if (list_empty(&transport->send_ready)) {
				list_add_tail(&transport->send_ready,
					      &transport_list);
			}
			packet = &transport->packet;
			sctp_packet_config(packet, vtag,
					   asoc->peer.ecn_capable);
		}

		switch (chunk->chunk_hdr->type) {
		/*
		 * 6.10 Bundling
		 *   ...
		 *   An endpoint MUST NOT bundle INIT, INIT ACK or SHUTDOWN
		 *   COMPLETE with any other chunks.  [Send them immediately.]
		 */
		case SCTP_CID_INIT:
		case SCTP_CID_INIT_ACK:
		case SCTP_CID_SHUTDOWN_COMPLETE:
			sctp_packet_init(&singleton, transport, sport, dport);
			sctp_packet_config(&singleton, vtag, 0);
			sctp_packet_append_chunk(&singleton, chunk);
			error = sctp_packet_transmit(&singleton);
			if (error < 0)
				return error;
			break;

		case SCTP_CID_ABORT:
			if (sctp_test_T_bit(chunk)) {
				packet->vtag = asoc->c.my_vtag;
			}
		/* The following chunks are "response" chunks, i.e.
		 * they are generated in response to something we
		 * received.  If we are sending these, then we can
		 * send only 1 packet containing these chunks.
		 */
		case SCTP_CID_HEARTBEAT_ACK:
		case SCTP_CID_SHUTDOWN_ACK:
		case SCTP_CID_COOKIE_ACK:
		case SCTP_CID_COOKIE_ECHO:
		case SCTP_CID_ERROR:
		case SCTP_CID_ECN_CWR:
		case SCTP_CID_ASCONF_ACK:
			one_packet = 1;
			/* Fall through */

		case SCTP_CID_SACK:
		case SCTP_CID_HEARTBEAT:
		case SCTP_CID_SHUTDOWN:
		case SCTP_CID_ECN_ECNE:
		case SCTP_CID_ASCONF:
		case SCTP_CID_FWD_TSN:
			status = sctp_packet_transmit_chunk(packet, chunk,
							    one_packet);
			if (status  != SCTP_XMIT_OK) {
				/* put the chunk back */
				list_add(&chunk->list, &q->control_chunk_list);
			} else if (chunk->chunk_hdr->type == SCTP_CID_FWD_TSN) {
				/* PR-SCTP C5) If a FORWARD TSN is sent, the
				 * sender MUST assure that at least one T3-rtx
				 * timer is running.
				 */
				sctp_transport_reset_timers(transport);
			}
			break;

		default:
			/* We built a chunk with an illegal type! */
			BUG();
		}
	}

	if (q->asoc->src_out_of_asoc_ok)
		goto sctp_flush_out;

	/* Is it OK to send data chunks?  */
	switch (asoc->state) {
	case SCTP_STATE_COOKIE_ECHOED:
		/* Only allow bundling when this packet has a COOKIE-ECHO
		 * chunk.
		 */
		if (!packet || !packet->has_cookie_echo)
			break;

		/* fallthru */
	case SCTP_STATE_ESTABLISHED:
	case SCTP_STATE_SHUTDOWN_PENDING:
	case SCTP_STATE_SHUTDOWN_RECEIVED:
		/*
		 * RFC 2960 6.1  Transmission of DATA Chunks
		 *
		 * C) When the time comes for the sender to transmit,
		 * before sending new DATA chunks, the sender MUST
		 * first transmit any outstanding DATA chunks which
		 * are marked for retransmission (limited by the
		 * current cwnd).
		 */
		if (!list_empty(&q->retransmit)) {
			if (asoc->peer.retran_path->state == SCTP_UNCONFIRMED)
				goto sctp_flush_out;
			if (transport == asoc->peer.retran_path)
				goto retran;

			/* Switch transports & prepare the packet.  */

			transport = asoc->peer.retran_path;

			if (list_empty(&transport->send_ready)) {
				list_add_tail(&transport->send_ready,
					      &transport_list);
			}

			packet = &transport->packet;
			sctp_packet_config(packet, vtag,
					   asoc->peer.ecn_capable);
		retran:
			error = sctp_outq_flush_rtx(q, packet,
						    rtx_timeout, &start_timer);

			if (start_timer)
				sctp_transport_reset_timers(transport);

			/* This can happen on COOKIE-ECHO resend.  Only
			 * one chunk can get bundled with a COOKIE-ECHO.
			 */
			if (packet->has_cookie_echo)
				goto sctp_flush_out;

			/* Don't send new data if there is still data
			 * waiting to retransmit.
			 */
			if (!list_empty(&q->retransmit))
				goto sctp_flush_out;
		}

		/* Apply Max.Burst limitation to the current transport in
		 * case it will be used for new data.  We are going to
		 * rest it before we return, but we want to apply the limit
		 * to the currently queued data.
		 */
		if (transport)
			sctp_transport_burst_limited(transport);

		/* Finally, transmit new packets.  */
		while ((chunk = sctp_outq_dequeue_data(q)) != NULL) {
			/* RFC 2960 6.5 Every DATA chunk MUST carry a valid
			 * stream identifier.
			 */
			if (chunk->sinfo.sinfo_stream >=
			    asoc->c.sinit_num_ostreams) {

				/* Mark as failed send. */
				sctp_chunk_fail(chunk, SCTP_ERROR_INV_STRM);
				sctp_chunk_free(chunk);
				continue;
			}

			/* Has this chunk expired? */
			if (sctp_chunk_abandoned(chunk)) {
				sctp_chunk_fail(chunk, 0);
				sctp_chunk_free(chunk);
				continue;
			}

			/* If there is a specified transport, use it.
			 * Otherwise, we want to use the active path.
			 */
			new_transport = chunk->transport;
			if (!new_transport ||
			    ((new_transport->state == SCTP_INACTIVE) ||
			     (new_transport->state == SCTP_UNCONFIRMED) ||
			     (new_transport->state == SCTP_PF)))
				new_transport = asoc->peer.active_path;
			if (new_transport->state == SCTP_UNCONFIRMED)
				continue;

			/* Change packets if necessary.  */
			if (new_transport != transport) {
				transport = new_transport;

				/* Schedule to have this transport's
				 * packet flushed.
				 */
				if (list_empty(&transport->send_ready)) {
					list_add_tail(&transport->send_ready,
						      &transport_list);
				}

				packet = &transport->packet;
				sctp_packet_config(packet, vtag,
						   asoc->peer.ecn_capable);
				/* We've switched transports, so apply the
				 * Burst limit to the new transport.
				 */
				sctp_transport_burst_limited(transport);
			}

			SCTP_DEBUG_PRINTK("sctp_outq_flush(%p, %p[%s]), ",
					  q, chunk,
					  chunk && chunk->chunk_hdr ?
					  sctp_cname(SCTP_ST_CHUNK(
						  chunk->chunk_hdr->type))
					  : "Illegal Chunk");

			SCTP_DEBUG_PRINTK("TX TSN 0x%x skb->head "
					"%p skb->users %d.\n",
					ntohl(chunk->subh.data_hdr->tsn),
					chunk->skb ?chunk->skb->head : NULL,
					chunk->skb ?
					atomic_read(&chunk->skb->users) : -1);

			/* Add the chunk to the packet.  */
			status = sctp_packet_transmit_chunk(packet, chunk, 0);

			switch (status) {
			case SCTP_XMIT_PMTU_FULL:
			case SCTP_XMIT_RWND_FULL:
			case SCTP_XMIT_NAGLE_DELAY:
				/* We could not append this chunk, so put
				 * the chunk back on the output queue.
				 */
				SCTP_DEBUG_PRINTK("sctp_outq_flush: could "
					"not transmit TSN: 0x%x, status: %d\n",
					ntohl(chunk->subh.data_hdr->tsn),
					status);
				sctp_outq_head_data(q, chunk);
				goto sctp_flush_out;
				break;

			case SCTP_XMIT_OK:
				/* The sender is in the SHUTDOWN-PENDING state,
				 * The sender MAY set the I-bit in the DATA
				 * chunk header.
				 */
				if (asoc->state == SCTP_STATE_SHUTDOWN_PENDING)
					chunk->chunk_hdr->flags |= SCTP_DATA_SACK_IMM;

				break;

			default:
				BUG();
			}

			/* BUG: We assume that the sctp_packet_transmit()
			 * call below will succeed all the time and add the
			 * chunk to the transmitted list and restart the
			 * timers.
			 * It is possible that the call can fail under OOM
			 * conditions.
			 *
			 * Is this really a problem?  Won't this behave
			 * like a lost TSN?
			 */
			list_add_tail(&chunk->transmitted_list,
				      &transport->transmitted);

			sctp_transport_reset_timers(transport);

			q->empty = 0;

			/* Only let one DATA chunk get bundled with a
			 * COOKIE-ECHO chunk.
			 */
			if (packet->has_cookie_echo)
				goto sctp_flush_out;
		}
		break;

	default:
		/* Do nothing.  */
		break;
	}

sctp_flush_out:

	/* Before returning, examine all the transports touched in
	 * this call.  Right now, we bluntly force clear all the
	 * transports.  Things might change after we implement Nagle.
	 * But such an examination is still required.
	 *
	 * --xguo
	 */
	while ((ltransport = sctp_list_dequeue(&transport_list)) != NULL ) {
		struct sctp_transport *t = list_entry(ltransport,
						      struct sctp_transport,
						      send_ready);
		packet = &t->packet;
		if (!sctp_packet_empty(packet))
			error = sctp_packet_transmit(packet);

		/* Clear the burst limited state, if any */
		sctp_transport_burst_reset(t);
	}

	return error;
}