static void sctp_endpoint_bh_rcv(struct work_struct *work)
{
	struct sctp_endpoint *ep =
		container_of(work, struct sctp_endpoint,
			     base.inqueue.immediate);
	struct sctp_association *asoc;
	struct sock *sk;
	struct net *net;
	struct sctp_transport *transport;
	struct sctp_chunk *chunk;
	struct sctp_inq *inqueue;
	sctp_subtype_t subtype;
	sctp_state_t state;
	int error = 0;
	int first_time = 1;	/* is this the first time through the loop */

	if (ep->base.dead)
		return;

	asoc = NULL;
	inqueue = &ep->base.inqueue;
	sk = ep->base.sk;
	net = sock_net(sk);

	while (NULL != (chunk = sctp_inq_pop(inqueue))) {
		subtype = SCTP_ST_CHUNK(chunk->chunk_hdr->type);

		/* If the first chunk in the packet is AUTH, do special
		 * processing specified in Section 6.3 of SCTP-AUTH spec
		 */
		if (first_time && (subtype.chunk == SCTP_CID_AUTH)) {
			struct sctp_chunkhdr *next_hdr;

			next_hdr = sctp_inq_peek(inqueue);
			if (!next_hdr)
				goto normal;

			/* If the next chunk is COOKIE-ECHO, skip the AUTH
			 * chunk while saving a pointer to it so we can do
			 * Authentication later (during cookie-echo
			 * processing).
			 */
			if (next_hdr->type == SCTP_CID_COOKIE_ECHO) {
				chunk->auth_chunk = skb_clone(chunk->skb,
								GFP_ATOMIC);
				chunk->auth = 1;
				continue;
			}
		}
normal:
		/* We might have grown an association since last we
		 * looked, so try again.
		 *
		 * This happens when we've just processed our
		 * COOKIE-ECHO chunk.
		 */
		if (NULL == chunk->asoc) {
			asoc = sctp_endpoint_lookup_assoc(ep,
							  sctp_source(chunk),
							  &transport);
			chunk->asoc = asoc;
			chunk->transport = transport;
		}

		state = asoc ? asoc->state : SCTP_STATE_CLOSED;
		if (sctp_auth_recv_cid(subtype.chunk, asoc) && !chunk->auth)
			continue;

		/* Remember where the last DATA chunk came from so we
		 * know where to send the SACK.
		 */
		if (asoc && sctp_chunk_is_data(chunk))
			asoc->peer.last_data_from = chunk->transport;
		else
			SCTP_INC_STATS(sock_net(ep->base.sk), SCTP_MIB_INCTRLCHUNKS);

		if (chunk->transport)
			chunk->transport->last_time_heard = jiffies;

		error = sctp_do_sm(net, SCTP_EVENT_T_CHUNK, subtype, state,
				   ep, asoc, chunk, GFP_ATOMIC);

		if (error && chunk)
			chunk->pdiscard = 1;

		/* Check to see if the endpoint is freed in response to
		 * the incoming chunk. If so, get out of the while loop.
		 */
		if (!sctp_sk(sk)->ep)
			break;

		if (first_time)
			first_time = 0;
	}
}