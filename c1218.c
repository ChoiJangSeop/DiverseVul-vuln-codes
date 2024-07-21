int sctp_outq_sack(struct sctp_outq *q, struct sctp_chunk *chunk)
{
	struct sctp_association *asoc = q->asoc;
	struct sctp_sackhdr *sack = chunk->subh.sack_hdr;
	struct sctp_transport *transport;
	struct sctp_chunk *tchunk = NULL;
	struct list_head *lchunk, *transport_list, *temp;
	sctp_sack_variable_t *frags = sack->variable;
	__u32 sack_ctsn, ctsn, tsn;
	__u32 highest_tsn, highest_new_tsn;
	__u32 sack_a_rwnd;
	unsigned int outstanding;
	struct sctp_transport *primary = asoc->peer.primary_path;
	int count_of_newacks = 0;
	int gap_ack_blocks;
	u8 accum_moved = 0;

	/* Grab the association's destination address list. */
	transport_list = &asoc->peer.transport_addr_list;

	sack_ctsn = ntohl(sack->cum_tsn_ack);
	gap_ack_blocks = ntohs(sack->num_gap_ack_blocks);
	/*
	 * SFR-CACC algorithm:
	 * On receipt of a SACK the sender SHOULD execute the
	 * following statements.
	 *
	 * 1) If the cumulative ack in the SACK passes next tsn_at_change
	 * on the current primary, the CHANGEOVER_ACTIVE flag SHOULD be
	 * cleared. The CYCLING_CHANGEOVER flag SHOULD also be cleared for
	 * all destinations.
	 * 2) If the SACK contains gap acks and the flag CHANGEOVER_ACTIVE
	 * is set the receiver of the SACK MUST take the following actions:
	 *
	 * A) Initialize the cacc_saw_newack to 0 for all destination
	 * addresses.
	 *
	 * Only bother if changeover_active is set. Otherwise, this is
	 * totally suboptimal to do on every SACK.
	 */
	if (primary->cacc.changeover_active) {
		u8 clear_cycling = 0;

		if (TSN_lte(primary->cacc.next_tsn_at_change, sack_ctsn)) {
			primary->cacc.changeover_active = 0;
			clear_cycling = 1;
		}

		if (clear_cycling || gap_ack_blocks) {
			list_for_each_entry(transport, transport_list,
					transports) {
				if (clear_cycling)
					transport->cacc.cycling_changeover = 0;
				if (gap_ack_blocks)
					transport->cacc.cacc_saw_newack = 0;
			}
		}
	}

	/* Get the highest TSN in the sack. */
	highest_tsn = sack_ctsn;
	if (gap_ack_blocks)
		highest_tsn += ntohs(frags[gap_ack_blocks - 1].gab.end);

	if (TSN_lt(asoc->highest_sacked, highest_tsn))
		asoc->highest_sacked = highest_tsn;

	highest_new_tsn = sack_ctsn;

	/* Run through the retransmit queue.  Credit bytes received
	 * and free those chunks that we can.
	 */
	sctp_check_transmitted(q, &q->retransmit, NULL, NULL, sack, &highest_new_tsn);

	/* Run through the transmitted queue.
	 * Credit bytes received and free those chunks which we can.
	 *
	 * This is a MASSIVE candidate for optimization.
	 */
	list_for_each_entry(transport, transport_list, transports) {
		sctp_check_transmitted(q, &transport->transmitted,
				       transport, &chunk->source, sack,
				       &highest_new_tsn);
		/*
		 * SFR-CACC algorithm:
		 * C) Let count_of_newacks be the number of
		 * destinations for which cacc_saw_newack is set.
		 */
		if (transport->cacc.cacc_saw_newack)
			count_of_newacks ++;
	}

	/* Move the Cumulative TSN Ack Point if appropriate.  */
	if (TSN_lt(asoc->ctsn_ack_point, sack_ctsn)) {
		asoc->ctsn_ack_point = sack_ctsn;
		accum_moved = 1;
	}

	if (gap_ack_blocks) {

		if (asoc->fast_recovery && accum_moved)
			highest_new_tsn = highest_tsn;

		list_for_each_entry(transport, transport_list, transports)
			sctp_mark_missing(q, &transport->transmitted, transport,
					  highest_new_tsn, count_of_newacks);
	}

	/* Update unack_data field in the assoc. */
	sctp_sack_update_unack_data(asoc, sack);

	ctsn = asoc->ctsn_ack_point;

	/* Throw away stuff rotting on the sack queue.  */
	list_for_each_safe(lchunk, temp, &q->sacked) {
		tchunk = list_entry(lchunk, struct sctp_chunk,
				    transmitted_list);
		tsn = ntohl(tchunk->subh.data_hdr->tsn);
		if (TSN_lte(tsn, ctsn)) {
			list_del_init(&tchunk->transmitted_list);
			sctp_chunk_free(tchunk);
		}
	}

	/* ii) Set rwnd equal to the newly received a_rwnd minus the
	 *     number of bytes still outstanding after processing the
	 *     Cumulative TSN Ack and the Gap Ack Blocks.
	 */

	sack_a_rwnd = ntohl(sack->a_rwnd);
	outstanding = q->outstanding_bytes;

	if (outstanding < sack_a_rwnd)
		sack_a_rwnd -= outstanding;
	else
		sack_a_rwnd = 0;

	asoc->peer.rwnd = sack_a_rwnd;

	sctp_generate_fwdtsn(q, sack_ctsn);

	SCTP_DEBUG_PRINTK("%s: sack Cumulative TSN Ack is 0x%x.\n",
			  __func__, sack_ctsn);
	SCTP_DEBUG_PRINTK("%s: Cumulative TSN Ack of association, "
			  "%p is 0x%x. Adv peer ack point: 0x%x\n",
			  __func__, asoc, ctsn, asoc->adv_peer_ack_point);

	/* See if all chunks are acked.
	 * Make sure the empty queue handler will get run later.
	 */
	q->empty = (list_empty(&q->out_chunk_list) &&
		    list_empty(&q->retransmit));
	if (!q->empty)
		goto finish;

	list_for_each_entry(transport, transport_list, transports) {
		q->empty = q->empty && list_empty(&transport->transmitted);
		if (!q->empty)
			goto finish;
	}

	SCTP_DEBUG_PRINTK("sack queue is empty.\n");
finish:
	return q->empty;
}