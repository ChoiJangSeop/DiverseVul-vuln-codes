struct sctp_chunk *sctp_make_sack(const struct sctp_association *asoc)
{
	struct sctp_chunk *retval;
	struct sctp_sackhdr sack;
	int len;
	__u32 ctsn;
	__u16 num_gabs, num_dup_tsns;
	struct sctp_association *aptr = (struct sctp_association *)asoc;
	struct sctp_tsnmap *map = (struct sctp_tsnmap *)&asoc->peer.tsn_map;
	struct sctp_gap_ack_block gabs[SCTP_MAX_GABS];
	struct sctp_transport *trans;

	memset(gabs, 0, sizeof(gabs));
	ctsn = sctp_tsnmap_get_ctsn(map);
	SCTP_DEBUG_PRINTK("sackCTSNAck sent:  0x%x.\n", ctsn);

	/* How much room is needed in the chunk? */
	num_gabs = sctp_tsnmap_num_gabs(map, gabs);
	num_dup_tsns = sctp_tsnmap_num_dups(map);

	/* Initialize the SACK header.  */
	sack.cum_tsn_ack	    = htonl(ctsn);
	sack.a_rwnd 		    = htonl(asoc->a_rwnd);
	sack.num_gap_ack_blocks     = htons(num_gabs);
	sack.num_dup_tsns           = htons(num_dup_tsns);

	len = sizeof(sack)
		+ sizeof(struct sctp_gap_ack_block) * num_gabs
		+ sizeof(__u32) * num_dup_tsns;

	/* Create the chunk.  */
	retval = sctp_make_chunk(asoc, SCTP_CID_SACK, 0, len);
	if (!retval)
		goto nodata;

	/* RFC 2960 6.4 Multi-homed SCTP Endpoints
	 *
	 * An endpoint SHOULD transmit reply chunks (e.g., SACK,
	 * HEARTBEAT ACK, etc.) to the same destination transport
	 * address from which it received the DATA or control chunk to
	 * which it is replying.  This rule should also be followed if
	 * the endpoint is bundling DATA chunks together with the
	 * reply chunk.
	 *
	 * However, when acknowledging multiple DATA chunks received
	 * in packets from different source addresses in a single
	 * SACK, the SACK chunk may be transmitted to one of the
	 * destination transport addresses from which the DATA or
	 * control chunks being acknowledged were received.
	 *
	 * [BUG:  We do not implement the following paragraph.
	 * Perhaps we should remember the last transport we used for a
	 * SACK and avoid that (if possible) if we have seen any
	 * duplicates. --piggy]
	 *
	 * When a receiver of a duplicate DATA chunk sends a SACK to a
	 * multi- homed endpoint it MAY be beneficial to vary the
	 * destination address and not use the source address of the
	 * DATA chunk.  The reason being that receiving a duplicate
	 * from a multi-homed endpoint might indicate that the return
	 * path (as specified in the source address of the DATA chunk)
	 * for the SACK is broken.
	 *
	 * [Send to the address from which we last received a DATA chunk.]
	 */
	retval->transport = asoc->peer.last_data_from;

	retval->subh.sack_hdr =
		sctp_addto_chunk(retval, sizeof(sack), &sack);

	/* Add the gap ack block information.   */
	if (num_gabs)
		sctp_addto_chunk(retval, sizeof(__u32) * num_gabs,
				 gabs);

	/* Add the duplicate TSN information.  */
	if (num_dup_tsns)
		sctp_addto_chunk(retval, sizeof(__u32) * num_dup_tsns,
				 sctp_tsnmap_get_dups(map));

	/* Once we have a sack generated, check to see what our sack
	 * generation is, if its 0, reset the transports to 0, and reset
	 * the association generation to 1
	 *
	 * The idea is that zero is never used as a valid generation for the
	 * association so no transport will match after a wrap event like this,
	 * Until the next sack
	 */
	if (++aptr->peer.sack_generation == 0) {
		list_for_each_entry(trans, &asoc->peer.transport_addr_list,
				    transports)
			trans->sack_generation = 0;
		aptr->peer.sack_generation = 1;
	}
nodata:
	return retval;
}