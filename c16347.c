dtls1_process_out_of_seq_message(SSL *s, struct hm_header_st* msg_hdr, int *ok)
{
	int i=-1;
	hm_fragment *frag = NULL;
	pitem *item = NULL;
	PQ_64BIT seq64;
	unsigned long frag_len = msg_hdr->frag_len;

	if ((msg_hdr->frag_off+frag_len) > msg_hdr->msg_len)
		goto err;

	/* Try to find item in queue, to prevent duplicate entries */
	pq_64bit_init(&seq64);
	pq_64bit_assign_word(&seq64, msg_hdr->seq);
	item = pqueue_find(s->d1->buffered_messages, seq64);
	pq_64bit_free(&seq64);
	
	/* Discard the message if sequence number was already there, is
	 * too far in the future, already in the queue or if we received
	 * a FINISHED before the SERVER_HELLO, which then must be a stale
	 * retransmit.
	 */
	if (msg_hdr->seq <= s->d1->handshake_read_seq ||
		msg_hdr->seq > s->d1->handshake_read_seq + 10 || item != NULL ||
		(s->d1->handshake_read_seq == 0 && msg_hdr->type == SSL3_MT_FINISHED))
		{
		unsigned char devnull [256];

		while (frag_len)
			{
			i = s->method->ssl_read_bytes(s,SSL3_RT_HANDSHAKE,
				devnull,
				frag_len>sizeof(devnull)?sizeof(devnull):frag_len,0);
			if (i<=0) goto err;
			frag_len -= i;
			}
		}

	if (frag_len)
	{
		frag = dtls1_hm_fragment_new(frag_len);
		if ( frag == NULL)
			goto err;

		memcpy(&(frag->msg_header), msg_hdr, sizeof(*msg_hdr));

		/* read the body of the fragment (header has already been read) */
		i = s->method->ssl_read_bytes(s,SSL3_RT_HANDSHAKE,
			frag->fragment,frag_len,0);
		if (i<=0 || (unsigned long)i!=frag_len)
			goto err;

		pq_64bit_init(&seq64);
		pq_64bit_assign_word(&seq64, msg_hdr->seq);

		item = pitem_new(seq64, frag);
		pq_64bit_free(&seq64);
		if ( item == NULL)
			goto err;

		pqueue_insert(s->d1->buffered_messages, item);
	}

	return DTLS1_HM_FRAGMENT_RETRY;

err:
	if ( frag != NULL) dtls1_hm_fragment_free(frag);
	if ( item != NULL) OPENSSL_free(item);
	*ok = 0;
	return i;
	}