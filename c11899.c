static int sctp_eat_data(const struct sctp_association *asoc,
			 struct sctp_chunk *chunk,
			 sctp_cmd_seq_t *commands)
{
	sctp_datahdr_t *data_hdr;
	struct sctp_chunk *err;
	size_t datalen;
	sctp_verb_t deliver;
	int tmp;
	__u32 tsn;
	int account_value;
	struct sock *sk = asoc->base.sk;

	data_hdr = chunk->subh.data_hdr = (sctp_datahdr_t *)chunk->skb->data;
	skb_pull(chunk->skb, sizeof(sctp_datahdr_t));

	tsn = ntohl(data_hdr->tsn);
	SCTP_DEBUG_PRINTK("eat_data: TSN 0x%x.\n", tsn);

	/* ASSERT:  Now skb->data is really the user data.  */

	/*
	 * if we are established, and we have used up our receive
	 * buffer memory, drop the frame
	 */
	if (asoc->state == SCTP_STATE_ESTABLISHED) {
		/*
		 * If the receive buffer policy is 1, then each
		 * association can allocate up to sk_rcvbuf bytes
		 * otherwise, all the associations in aggregate
		 * may allocate up to sk_rcvbuf bytes
		 */
		if (asoc->ep->rcvbuf_policy)
			account_value = atomic_read(&asoc->rmem_alloc);
		else
			account_value = atomic_read(&sk->sk_rmem_alloc);

		if (account_value > sk->sk_rcvbuf)
			return SCTP_IERROR_IGNORE_TSN;
	}

	/* Process ECN based congestion.
	 *
	 * Since the chunk structure is reused for all chunks within
	 * a packet, we use ecn_ce_done to track if we've already
	 * done CE processing for this packet.
	 *
	 * We need to do ECN processing even if we plan to discard the
	 * chunk later.
	 */

	if (!chunk->ecn_ce_done) {
		struct sctp_af *af;
		chunk->ecn_ce_done = 1;

		af = sctp_get_af_specific(
			ipver2af(chunk->skb->nh.iph->version));

		if (af && af->is_ce(chunk->skb) && asoc->peer.ecn_capable) {
			/* Do real work as sideffect. */
			sctp_add_cmd_sf(commands, SCTP_CMD_ECN_CE,
					SCTP_U32(tsn));
		}
	}

	tmp = sctp_tsnmap_check(&asoc->peer.tsn_map, tsn);
	if (tmp < 0) {
		/* The TSN is too high--silently discard the chunk and
		 * count on it getting retransmitted later.
		 */
		return SCTP_IERROR_HIGH_TSN;
	} else if (tmp > 0) {
		/* This is a duplicate.  Record it.  */
		sctp_add_cmd_sf(commands, SCTP_CMD_REPORT_DUP, SCTP_U32(tsn));
		return SCTP_IERROR_DUP_TSN;
	}

	/* This is a new TSN.  */

	/* Discard if there is no room in the receive window.
	 * Actually, allow a little bit of overflow (up to a MTU).
	 */
	datalen = ntohs(chunk->chunk_hdr->length);
	datalen -= sizeof(sctp_data_chunk_t);

	deliver = SCTP_CMD_CHUNK_ULP;

	/* Think about partial delivery. */
	if ((datalen >= asoc->rwnd) && (!asoc->ulpq.pd_mode)) {

		/* Even if we don't accept this chunk there is
		 * memory pressure.
		 */
		sctp_add_cmd_sf(commands, SCTP_CMD_PART_DELIVER, SCTP_NULL());
	}

        /* Spill over rwnd a little bit.  Note: While allowed, this spill over
	 * seems a bit troublesome in that frag_point varies based on
	 * PMTU.  In cases, such as loopback, this might be a rather
	 * large spill over.
	 */
	if (!asoc->rwnd || asoc->rwnd_over ||
	    (datalen > asoc->rwnd + asoc->frag_point)) {

		/* If this is the next TSN, consider reneging to make
		 * room.   Note: Playing nice with a confused sender.  A
		 * malicious sender can still eat up all our buffer
		 * space and in the future we may want to detect and
		 * do more drastic reneging.
		 */
		if (sctp_tsnmap_has_gap(&asoc->peer.tsn_map) &&
		    (sctp_tsnmap_get_ctsn(&asoc->peer.tsn_map) + 1) == tsn) {
			SCTP_DEBUG_PRINTK("Reneging for tsn:%u\n", tsn);
			deliver = SCTP_CMD_RENEGE;
		} else {
			SCTP_DEBUG_PRINTK("Discard tsn: %u len: %Zd, "
					  "rwnd: %d\n", tsn, datalen,
					  asoc->rwnd);
			return SCTP_IERROR_IGNORE_TSN;
		}
	}

	/*
	 * Section 3.3.10.9 No User Data (9)
	 *
	 * Cause of error
	 * ---------------
	 * No User Data:  This error cause is returned to the originator of a
	 * DATA chunk if a received DATA chunk has no user data.
	 */
	if (unlikely(0 == datalen)) {
		err = sctp_make_abort_no_data(asoc, chunk, tsn);
		if (err) {
			sctp_add_cmd_sf(commands, SCTP_CMD_REPLY,
					SCTP_CHUNK(err));
		}
		/* We are going to ABORT, so we might as well stop
		 * processing the rest of the chunks in the packet.
		 */
		sctp_add_cmd_sf(commands, SCTP_CMD_DISCARD_PACKET,SCTP_NULL());
		sctp_add_cmd_sf(commands, SCTP_CMD_ASSOC_FAILED,
				SCTP_U32(SCTP_ERROR_NO_DATA));
		SCTP_INC_STATS(SCTP_MIB_ABORTEDS);
		SCTP_DEC_STATS(SCTP_MIB_CURRESTAB);
		return SCTP_IERROR_NO_DATA;
	}

	/* If definately accepting the DATA chunk, record its TSN, otherwise
	 * wait for renege processing.
	 */
	if (SCTP_CMD_CHUNK_ULP == deliver)
		sctp_add_cmd_sf(commands, SCTP_CMD_REPORT_TSN, SCTP_U32(tsn));

	/* Note: Some chunks may get overcounted (if we drop) or overcounted
	 * if we renege and the chunk arrives again.
	 */
	if (chunk->chunk_hdr->flags & SCTP_DATA_UNORDERED)
		SCTP_INC_STATS(SCTP_MIB_INUNORDERCHUNKS);
	else
		SCTP_INC_STATS(SCTP_MIB_INORDERCHUNKS);

	/* RFC 2960 6.5 Stream Identifier and Stream Sequence Number
	 *
	 * If an endpoint receive a DATA chunk with an invalid stream
	 * identifier, it shall acknowledge the reception of the DATA chunk
	 * following the normal procedure, immediately send an ERROR chunk
	 * with cause set to "Invalid Stream Identifier" (See Section 3.3.10)
	 * and discard the DATA chunk.
	 */
	if (ntohs(data_hdr->stream) >= asoc->c.sinit_max_instreams) {
		err = sctp_make_op_error(asoc, chunk, SCTP_ERROR_INV_STRM,
					 &data_hdr->stream,
					 sizeof(data_hdr->stream));
		if (err)
			sctp_add_cmd_sf(commands, SCTP_CMD_REPLY,
					SCTP_CHUNK(err));
		return SCTP_IERROR_BAD_STREAM;
	}

	/* Send the data up to the user.  Note:  Schedule  the
	 * SCTP_CMD_CHUNK_ULP cmd before the SCTP_CMD_GEN_SACK, as the SACK
	 * chunk needs the updated rwnd.
	 */
	sctp_add_cmd_sf(commands, deliver, SCTP_CHUNK(chunk));

	return SCTP_IERROR_NO_ERROR;
}