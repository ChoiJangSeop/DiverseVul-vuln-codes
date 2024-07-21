static sctp_xmit_t __sctp_packet_append_chunk(struct sctp_packet *packet,
					      struct sctp_chunk *chunk)
{
	sctp_xmit_t retval = SCTP_XMIT_OK;
	__u16 chunk_len = WORD_ROUND(ntohs(chunk->chunk_hdr->length));

	/* Check to see if this chunk will fit into the packet */
	retval = sctp_packet_will_fit(packet, chunk, chunk_len);
	if (retval != SCTP_XMIT_OK)
		goto finish;

	/* We believe that this chunk is OK to add to the packet */
	switch (chunk->chunk_hdr->type) {
	    case SCTP_CID_DATA:
		/* Account for the data being in the packet */
		sctp_packet_append_data(packet, chunk);
		/* Disallow SACK bundling after DATA. */
		packet->has_sack = 1;
		/* Disallow AUTH bundling after DATA */
		packet->has_auth = 1;
		/* Let it be knows that packet has DATA in it */
		packet->has_data = 1;
		/* timestamp the chunk for rtx purposes */
		chunk->sent_at = jiffies;
		break;
	    case SCTP_CID_COOKIE_ECHO:
		packet->has_cookie_echo = 1;
		break;

	    case SCTP_CID_SACK:
		packet->has_sack = 1;
		break;

	    case SCTP_CID_AUTH:
		packet->has_auth = 1;
		packet->auth = chunk;
		break;
	}

	/* It is OK to send this chunk.  */
	list_add_tail(&chunk->list, &packet->chunk_list);
	packet->size += chunk_len;
	chunk->transport = packet->transport;
finish:
	return retval;
}