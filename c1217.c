int sctp_packet_transmit(struct sctp_packet *packet)
{
	struct sctp_transport *tp = packet->transport;
	struct sctp_association *asoc = tp->asoc;
	struct sctphdr *sh;
	struct sk_buff *nskb;
	struct sctp_chunk *chunk, *tmp;
	struct sock *sk;
	int err = 0;
	int padding;		/* How much padding do we need?  */
	__u8 has_data = 0;
	struct dst_entry *dst = tp->dst;
	unsigned char *auth = NULL;	/* pointer to auth in skb data */
	__u32 cksum_buf_len = sizeof(struct sctphdr);

	SCTP_DEBUG_PRINTK("%s: packet:%p\n", __func__, packet);

	/* Do NOT generate a chunkless packet. */
	if (list_empty(&packet->chunk_list))
		return err;

	/* Set up convenience variables... */
	chunk = list_entry(packet->chunk_list.next, struct sctp_chunk, list);
	sk = chunk->skb->sk;

	/* Allocate the new skb.  */
	nskb = alloc_skb(packet->size + LL_MAX_HEADER, GFP_ATOMIC);
	if (!nskb)
		goto nomem;

	/* Make sure the outbound skb has enough header room reserved. */
	skb_reserve(nskb, packet->overhead + LL_MAX_HEADER);

	/* Set the owning socket so that we know where to get the
	 * destination IP address.
	 */
	sctp_packet_set_owner_w(nskb, sk);

	if (!sctp_transport_dst_check(tp)) {
		sctp_transport_route(tp, NULL, sctp_sk(sk));
		if (asoc && (asoc->param_flags & SPP_PMTUD_ENABLE)) {
			sctp_assoc_sync_pmtu(sk, asoc);
		}
	}
	dst = dst_clone(tp->dst);
	skb_dst_set(nskb, dst);
	if (!dst)
		goto no_route;

	/* Build the SCTP header.  */
	sh = (struct sctphdr *)skb_push(nskb, sizeof(struct sctphdr));
	skb_reset_transport_header(nskb);
	sh->source = htons(packet->source_port);
	sh->dest   = htons(packet->destination_port);

	/* From 6.8 Adler-32 Checksum Calculation:
	 * After the packet is constructed (containing the SCTP common
	 * header and one or more control or DATA chunks), the
	 * transmitter shall:
	 *
	 * 1) Fill in the proper Verification Tag in the SCTP common
	 *    header and initialize the checksum field to 0's.
	 */
	sh->vtag     = htonl(packet->vtag);
	sh->checksum = 0;

	/**
	 * 6.10 Bundling
	 *
	 *    An endpoint bundles chunks by simply including multiple
	 *    chunks in one outbound SCTP packet.  ...
	 */

	/**
	 * 3.2  Chunk Field Descriptions
	 *
	 * The total length of a chunk (including Type, Length and
	 * Value fields) MUST be a multiple of 4 bytes.  If the length
	 * of the chunk is not a multiple of 4 bytes, the sender MUST
	 * pad the chunk with all zero bytes and this padding is not
	 * included in the chunk length field.  The sender should
	 * never pad with more than 3 bytes.
	 *
	 * [This whole comment explains WORD_ROUND() below.]
	 */
	SCTP_DEBUG_PRINTK("***sctp_transmit_packet***\n");
	list_for_each_entry_safe(chunk, tmp, &packet->chunk_list, list) {
		list_del_init(&chunk->list);
		if (sctp_chunk_is_data(chunk)) {
			/* 6.3.1 C4) When data is in flight and when allowed
			 * by rule C5, a new RTT measurement MUST be made each
			 * round trip.  Furthermore, new RTT measurements
			 * SHOULD be made no more than once per round-trip
			 * for a given destination transport address.
			 */

			if (!tp->rto_pending) {
				chunk->rtt_in_progress = 1;
				tp->rto_pending = 1;
			}
			has_data = 1;
		}

		padding = WORD_ROUND(chunk->skb->len) - chunk->skb->len;
		if (padding)
			memset(skb_put(chunk->skb, padding), 0, padding);

		/* if this is the auth chunk that we are adding,
		 * store pointer where it will be added and put
		 * the auth into the packet.
		 */
		if (chunk == packet->auth)
			auth = skb_tail_pointer(nskb);

		cksum_buf_len += chunk->skb->len;
		memcpy(skb_put(nskb, chunk->skb->len),
			       chunk->skb->data, chunk->skb->len);

		SCTP_DEBUG_PRINTK("%s %p[%s] %s 0x%x, %s %d, %s %d, %s %d\n",
				  "*** Chunk", chunk,
				  sctp_cname(SCTP_ST_CHUNK(
					  chunk->chunk_hdr->type)),
				  chunk->has_tsn ? "TSN" : "No TSN",
				  chunk->has_tsn ?
				  ntohl(chunk->subh.data_hdr->tsn) : 0,
				  "length", ntohs(chunk->chunk_hdr->length),
				  "chunk->skb->len", chunk->skb->len,
				  "rtt_in_progress", chunk->rtt_in_progress);

		/*
		 * If this is a control chunk, this is our last
		 * reference. Free data chunks after they've been
		 * acknowledged or have failed.
		 */
		if (!sctp_chunk_is_data(chunk))
			sctp_chunk_free(chunk);
	}

	/* SCTP-AUTH, Section 6.2
	 *    The sender MUST calculate the MAC as described in RFC2104 [2]
	 *    using the hash function H as described by the MAC Identifier and
	 *    the shared association key K based on the endpoint pair shared key
	 *    described by the shared key identifier.  The 'data' used for the
	 *    computation of the AUTH-chunk is given by the AUTH chunk with its
	 *    HMAC field set to zero (as shown in Figure 6) followed by all
	 *    chunks that are placed after the AUTH chunk in the SCTP packet.
	 */
	if (auth)
		sctp_auth_calculate_hmac(asoc, nskb,
					(struct sctp_auth_chunk *)auth,
					GFP_ATOMIC);

	/* 2) Calculate the Adler-32 checksum of the whole packet,
	 *    including the SCTP common header and all the
	 *    chunks.
	 *
	 * Note: Adler-32 is no longer applicable, as has been replaced
	 * by CRC32-C as described in <draft-ietf-tsvwg-sctpcsum-02.txt>.
	 */
	if (!sctp_checksum_disable) {
		if (!(dst->dev->features & NETIF_F_SCTP_CSUM)) {
			__u32 crc32 = sctp_start_cksum((__u8 *)sh, cksum_buf_len);

			/* 3) Put the resultant value into the checksum field in the
			 *    common header, and leave the rest of the bits unchanged.
			 */
			sh->checksum = sctp_end_cksum(crc32);
		} else {
			/* no need to seed pseudo checksum for SCTP */
			nskb->ip_summed = CHECKSUM_PARTIAL;
			nskb->csum_start = (skb_transport_header(nskb) -
			                    nskb->head);
			nskb->csum_offset = offsetof(struct sctphdr, checksum);
		}
	}

	/* IP layer ECN support
	 * From RFC 2481
	 *  "The ECN-Capable Transport (ECT) bit would be set by the
	 *   data sender to indicate that the end-points of the
	 *   transport protocol are ECN-capable."
	 *
	 * Now setting the ECT bit all the time, as it should not cause
	 * any problems protocol-wise even if our peer ignores it.
	 *
	 * Note: The works for IPv6 layer checks this bit too later
	 * in transmission.  See IP6_ECN_flow_xmit().
	 */
	(*tp->af_specific->ecn_capable)(nskb->sk);

	/* Set up the IP options.  */
	/* BUG: not implemented
	 * For v4 this all lives somewhere in sk->sk_opt...
	 */

	/* Dump that on IP!  */
	if (asoc && asoc->peer.last_sent_to != tp) {
		/* Considering the multiple CPU scenario, this is a
		 * "correcter" place for last_sent_to.  --xguo
		 */
		asoc->peer.last_sent_to = tp;
	}

	if (has_data) {
		struct timer_list *timer;
		unsigned long timeout;

		/* Restart the AUTOCLOSE timer when sending data. */
		if (sctp_state(asoc, ESTABLISHED) && asoc->autoclose) {
			timer = &asoc->timers[SCTP_EVENT_TIMEOUT_AUTOCLOSE];
			timeout = asoc->timeouts[SCTP_EVENT_TIMEOUT_AUTOCLOSE];

			if (!mod_timer(timer, jiffies + timeout))
				sctp_association_hold(asoc);
		}
	}

	SCTP_DEBUG_PRINTK("***sctp_transmit_packet*** skb len %d\n",
			  nskb->len);

	nskb->local_df = packet->ipfragok;
	(*tp->af_specific->sctp_xmit)(nskb, tp);

out:
	sctp_packet_reset(packet);
	return err;
no_route:
	kfree_skb(nskb);
	IP_INC_STATS_BH(sock_net(asoc->base.sk), IPSTATS_MIB_OUTNOROUTES);

	/* FIXME: Returning the 'err' will effect all the associations
	 * associated with a socket, although only one of the paths of the
	 * association is unreachable.
	 * The real failure of a transport or association can be passed on
	 * to the user via notifications. So setting this error may not be
	 * required.
	 */
	 /* err = -EHOSTUNREACH; */
err:
	/* Control chunks are unreliable so just drop them.  DATA chunks
	 * will get resent or dropped later.
	 */

	list_for_each_entry_safe(chunk, tmp, &packet->chunk_list, list) {
		list_del_init(&chunk->list);
		if (!sctp_chunk_is_data(chunk))
			sctp_chunk_free(chunk);
	}
	goto out;
nomem:
	err = -ENOMEM;
	goto err;
}