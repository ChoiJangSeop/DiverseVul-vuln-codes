int sctp_process_init(struct sctp_association *asoc, sctp_cid_t cid,
		      const union sctp_addr *peer_addr,
		      sctp_init_chunk_t *peer_init, gfp_t gfp)
{
	union sctp_params param;
	struct sctp_transport *transport;
	struct list_head *pos, *temp;
	char *cookie;

	/* We must include the address that the INIT packet came from.
	 * This is the only address that matters for an INIT packet.
	 * When processing a COOKIE ECHO, we retrieve the from address
	 * of the INIT from the cookie.
	 */

	/* This implementation defaults to making the first transport
	 * added as the primary transport.  The source address seems to
	 * be a a better choice than any of the embedded addresses.
	 */
	if (peer_addr) {
		if(!sctp_assoc_add_peer(asoc, peer_addr, gfp, SCTP_ACTIVE))
			goto nomem;
	}

	/* Process the initialization parameters.  */
	sctp_walk_params(param, peer_init, init_hdr.params) {

		if (!sctp_process_param(asoc, param, peer_addr, gfp))
			goto clean_up;
	}

	/* AUTH: After processing the parameters, make sure that we
	 * have all the required info to potentially do authentications.
	 */
	if (asoc->peer.auth_capable && (!asoc->peer.peer_random ||
					!asoc->peer.peer_hmacs))
		asoc->peer.auth_capable = 0;

	/* In a non-backward compatible mode, if the peer claims
	 * support for ADD-IP but not AUTH,  the ADD-IP spec states
	 * that we MUST ABORT the association. Section 6.  The section
	 * also give us an option to silently ignore the packet, which
	 * is what we'll do here.
	 */
	if (!sctp_addip_noauth &&
	     (asoc->peer.asconf_capable && !asoc->peer.auth_capable)) {
		asoc->peer.addip_disabled_mask |= (SCTP_PARAM_ADD_IP |
						  SCTP_PARAM_DEL_IP |
						  SCTP_PARAM_SET_PRIMARY);
		asoc->peer.asconf_capable = 0;
		goto clean_up;
	}

	/* Walk list of transports, removing transports in the UNKNOWN state. */
	list_for_each_safe(pos, temp, &asoc->peer.transport_addr_list) {
		transport = list_entry(pos, struct sctp_transport, transports);
		if (transport->state == SCTP_UNKNOWN) {
			sctp_assoc_rm_peer(asoc, transport);
		}
	}

	/* The fixed INIT headers are always in network byte
	 * order.
	 */
	asoc->peer.i.init_tag =
		ntohl(peer_init->init_hdr.init_tag);
	asoc->peer.i.a_rwnd =
		ntohl(peer_init->init_hdr.a_rwnd);
	asoc->peer.i.num_outbound_streams =
		ntohs(peer_init->init_hdr.num_outbound_streams);
	asoc->peer.i.num_inbound_streams =
		ntohs(peer_init->init_hdr.num_inbound_streams);
	asoc->peer.i.initial_tsn =
		ntohl(peer_init->init_hdr.initial_tsn);

	/* Apply the upper bounds for output streams based on peer's
	 * number of inbound streams.
	 */
	if (asoc->c.sinit_num_ostreams  >
	    ntohs(peer_init->init_hdr.num_inbound_streams)) {
		asoc->c.sinit_num_ostreams =
			ntohs(peer_init->init_hdr.num_inbound_streams);
	}

	if (asoc->c.sinit_max_instreams >
	    ntohs(peer_init->init_hdr.num_outbound_streams)) {
		asoc->c.sinit_max_instreams =
			ntohs(peer_init->init_hdr.num_outbound_streams);
	}

	/* Copy Initiation tag from INIT to VT_peer in cookie.   */
	asoc->c.peer_vtag = asoc->peer.i.init_tag;

	/* Peer Rwnd   : Current calculated value of the peer's rwnd.  */
	asoc->peer.rwnd = asoc->peer.i.a_rwnd;

	/* Copy cookie in case we need to resend COOKIE-ECHO. */
	cookie = asoc->peer.cookie;
	if (cookie) {
		asoc->peer.cookie = kmemdup(cookie, asoc->peer.cookie_len, gfp);
		if (!asoc->peer.cookie)
			goto clean_up;
	}

	/* RFC 2960 7.2.1 The initial value of ssthresh MAY be arbitrarily
	 * high (for example, implementations MAY use the size of the receiver
	 * advertised window).
	 */
	list_for_each_entry(transport, &asoc->peer.transport_addr_list,
			transports) {
		transport->ssthresh = asoc->peer.i.a_rwnd;
	}

	/* Set up the TSN tracking pieces.  */
	sctp_tsnmap_init(&asoc->peer.tsn_map, SCTP_TSN_MAP_SIZE,
			 asoc->peer.i.initial_tsn);

	/* RFC 2960 6.5 Stream Identifier and Stream Sequence Number
	 *
	 * The stream sequence number in all the streams shall start
	 * from 0 when the association is established.  Also, when the
	 * stream sequence number reaches the value 65535 the next
	 * stream sequence number shall be set to 0.
	 */

	/* Allocate storage for the negotiated streams if it is not a temporary
	 * association.
	 */
	if (!asoc->temp) {
		int error;

		asoc->ssnmap = sctp_ssnmap_new(asoc->c.sinit_max_instreams,
					       asoc->c.sinit_num_ostreams, gfp);
		if (!asoc->ssnmap)
			goto clean_up;

		error = sctp_assoc_set_id(asoc, gfp);
		if (error)
			goto clean_up;
	}

	/* ADDIP Section 4.1 ASCONF Chunk Procedures
	 *
	 * When an endpoint has an ASCONF signaled change to be sent to the
	 * remote endpoint it should do the following:
	 * ...
	 * A2) A serial number should be assigned to the Chunk. The serial
	 * number should be a monotonically increasing number. All serial
	 * numbers are defined to be initialized at the start of the
	 * association to the same value as the Initial TSN.
	 */
	asoc->peer.addip_serial = asoc->peer.i.initial_tsn - 1;
	return 1;

clean_up:
	/* Release the transport structures. */
	list_for_each_safe(pos, temp, &asoc->peer.transport_addr_list) {
		transport = list_entry(pos, struct sctp_transport, transports);
		list_del_init(pos);
		sctp_transport_free(transport);
	}

	asoc->peer.transport_count = 0;

nomem:
	return 0;
}