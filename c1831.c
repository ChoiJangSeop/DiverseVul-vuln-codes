static sctp_disposition_t sctp_sf_abort_violation(
				     const struct sctp_endpoint *ep,
				     const struct sctp_association *asoc,
				     void *arg,
				     sctp_cmd_seq_t *commands,
				     const __u8 *payload,
				     const size_t paylen)
{
	struct sctp_packet *packet = NULL;
	struct sctp_chunk *chunk =  arg;
	struct sctp_chunk *abort = NULL;

	/* Make the abort chunk. */
	abort = sctp_make_abort_violation(asoc, chunk, payload, paylen);
	if (!abort)
		goto nomem;

	if (asoc) {
		sctp_add_cmd_sf(commands, SCTP_CMD_REPLY, SCTP_CHUNK(abort));
		SCTP_INC_STATS(SCTP_MIB_OUTCTRLCHUNKS);

		if (asoc->state <= SCTP_STATE_COOKIE_ECHOED) {
			sctp_add_cmd_sf(commands, SCTP_CMD_TIMER_STOP,
					SCTP_TO(SCTP_EVENT_TIMEOUT_T1_INIT));
			sctp_add_cmd_sf(commands, SCTP_CMD_SET_SK_ERR,
					SCTP_ERROR(ECONNREFUSED));
			sctp_add_cmd_sf(commands, SCTP_CMD_INIT_FAILED,
					SCTP_PERR(SCTP_ERROR_PROTO_VIOLATION));
		} else {
			sctp_add_cmd_sf(commands, SCTP_CMD_SET_SK_ERR,
					SCTP_ERROR(ECONNABORTED));
			sctp_add_cmd_sf(commands, SCTP_CMD_ASSOC_FAILED,
					SCTP_PERR(SCTP_ERROR_PROTO_VIOLATION));
			SCTP_DEC_STATS(SCTP_MIB_CURRESTAB);
		}
	} else {
		packet = sctp_ootb_pkt_new(asoc, chunk);

		if (!packet)
			goto nomem_pkt;

		if (sctp_test_T_bit(abort))
			packet->vtag = ntohl(chunk->sctp_hdr->vtag);

		abort->skb->sk = ep->base.sk;

		sctp_packet_append_chunk(packet, abort);

		sctp_add_cmd_sf(commands, SCTP_CMD_SEND_PKT,
			SCTP_PACKET(packet));

		SCTP_INC_STATS(SCTP_MIB_OUTCTRLCHUNKS);
	}

	sctp_sf_pdiscard(ep, asoc, SCTP_ST_CHUNK(0), arg, commands);

	SCTP_INC_STATS(SCTP_MIB_ABORTEDS);

	return SCTP_DISPOSITION_ABORT;

nomem_pkt:
	sctp_chunk_free(abort);
nomem:
	return SCTP_DISPOSITION_NOMEM;
}