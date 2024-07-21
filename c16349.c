dtls1_reassemble_fragment(SSL *s, struct hm_header_st* msg_hdr, int *ok)
	{
	hm_fragment *frag = NULL;
	pitem *item = NULL;
	int i = -1, is_complete;
	unsigned char seq64be[8];
	unsigned long frag_len = msg_hdr->frag_len, max_len;

	if ((msg_hdr->frag_off+frag_len) > msg_hdr->msg_len)
		goto err;

	/* Determine maximum allowed message size. Depends on (user set)
	 * maximum certificate length, but 16k is minimum.
	 */
	if (DTLS1_HM_HEADER_LENGTH + SSL3_RT_MAX_ENCRYPTED_LENGTH < s->max_cert_list)
		max_len = s->max_cert_list;
	else
		max_len = DTLS1_HM_HEADER_LENGTH + SSL3_RT_MAX_ENCRYPTED_LENGTH;

	if ((msg_hdr->frag_off+frag_len) > max_len)
		goto err;

	/* Try to find item in queue */
	memset(seq64be,0,sizeof(seq64be));
	seq64be[6] = (unsigned char) (msg_hdr->seq>>8);
	seq64be[7] = (unsigned char) msg_hdr->seq;
	item = pqueue_find(s->d1->buffered_messages, seq64be);

	if (item == NULL)
		{
		frag = dtls1_hm_fragment_new(msg_hdr->msg_len, 1);
		if ( frag == NULL)
			goto err;
		memcpy(&(frag->msg_header), msg_hdr, sizeof(*msg_hdr));
		frag->msg_header.frag_len = frag->msg_header.msg_len;
		frag->msg_header.frag_off = 0;
		}
	else
		frag = (hm_fragment*) item->data;

	/* If message is already reassembled, this must be a
	 * retransmit and can be dropped.
	 */
	if (frag->reassembly == NULL)
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
		return DTLS1_HM_FRAGMENT_RETRY;
		}

	/* read the body of the fragment (header has already been read */
	i = s->method->ssl_read_bytes(s,SSL3_RT_HANDSHAKE,
		frag->fragment + msg_hdr->frag_off,frag_len,0);
	if (i<=0 || (unsigned long)i!=frag_len)
		goto err;

	RSMBLY_BITMASK_MARK(frag->reassembly, (long)msg_hdr->frag_off,
	                    (long)(msg_hdr->frag_off + frag_len));

	RSMBLY_BITMASK_IS_COMPLETE(frag->reassembly, (long)msg_hdr->msg_len,
	                           is_complete);

	if (is_complete)
		{
		OPENSSL_free(frag->reassembly);
		frag->reassembly = NULL;
		}

	if (item == NULL)
		{
		memset(seq64be,0,sizeof(seq64be));
		seq64be[6] = (unsigned char)(msg_hdr->seq>>8);
		seq64be[7] = (unsigned char)(msg_hdr->seq);

		item = pitem_new(seq64be, frag);
		if (item == NULL)
			{
			i = -1;
			goto err;
			}

		pqueue_insert(s->d1->buffered_messages, item);
		}

	return DTLS1_HM_FRAGMENT_RETRY;

err:
	if (frag != NULL) dtls1_hm_fragment_free(frag);
	if (item != NULL) OPENSSL_free(item);
	*ok = 0;
	return i;
	}