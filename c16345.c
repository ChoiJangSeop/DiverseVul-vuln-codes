long dtls1_get_message(SSL *s, int st1, int stn, int mt, long max, int *ok)
	{
	int i, al;
	struct hm_header_st *msg_hdr;

	/* s3->tmp is used to store messages that are unexpected, caused
	 * by the absence of an optional handshake message */
	if (s->s3->tmp.reuse_message)
		{
		s->s3->tmp.reuse_message=0;
		if ((mt >= 0) && (s->s3->tmp.message_type != mt))
			{
			al=SSL_AD_UNEXPECTED_MESSAGE;
			SSLerr(SSL_F_DTLS1_GET_MESSAGE,SSL_R_UNEXPECTED_MESSAGE);
			goto f_err;
			}
		*ok=1;
		s->init_msg = s->init_buf->data + DTLS1_HM_HEADER_LENGTH;
		s->init_num = (int)s->s3->tmp.message_size;
		return s->init_num;
		}

	msg_hdr = &s->d1->r_msg_hdr;
	do
		{
		if ( msg_hdr->frag_off == 0)
			{
			/* s->d1->r_message_header.msg_len = 0; */
			memset(msg_hdr, 0x00, sizeof(struct hm_header_st));
			}

		i = dtls1_get_message_fragment(s, st1, stn, max, ok);
		if ( i == DTLS1_HM_BAD_FRAGMENT ||
			i == DTLS1_HM_FRAGMENT_RETRY)  /* bad fragment received */
			continue;
		else if ( i <= 0 && !*ok)
			return i;

		/* Note that s->init_sum is used as a counter summing
		 * up fragments' lengths: as soon as they sum up to
		 * handshake packet length, we assume we have got all
		 * the fragments. Overlapping fragments would cause
		 * premature termination, so we don't expect overlaps.
		 * Well, handling overlaps would require something more
		 * drastic. Indeed, as it is now there is no way to
		 * tell if out-of-order fragment from the middle was
		 * the last. '>=' is the best/least we can do to control
		 * the potential damage caused by malformed overlaps. */
		if ((unsigned int)s->init_num >= msg_hdr->msg_len)
			{
			unsigned char *p = (unsigned char *)s->init_buf->data;
			unsigned long msg_len = msg_hdr->msg_len;

			/* reconstruct message header as if it was
			 * sent in single fragment */
			*(p++) = msg_hdr->type;
			l2n3(msg_len,p);
			s2n (msg_hdr->seq,p);
			l2n3(0,p);
			l2n3(msg_len,p);
			if (s->client_version != DTLS1_BAD_VER)
				p       -= DTLS1_HM_HEADER_LENGTH,
				msg_len += DTLS1_HM_HEADER_LENGTH;

			ssl3_finish_mac(s, p, msg_len);
			if (s->msg_callback)
				s->msg_callback(0, s->version, SSL3_RT_HANDSHAKE,
					p, msg_len,
					s, s->msg_callback_arg);

			memset(msg_hdr, 0x00, sizeof(struct hm_header_st));

			s->d1->handshake_read_seq++;
			/* we just read a handshake message from the other side:
			 * this means that we don't need to retransmit of the
			 * buffered messages.  
			 * XDTLS: may be able clear out this
			 * buffer a little sooner (i.e if an out-of-order
			 * handshake message/record is received at the record
			 * layer.  
			 * XDTLS: exception is that the server needs to
			 * know that change cipher spec and finished messages
			 * have been received by the client before clearing this
			 * buffer.  this can simply be done by waiting for the
			 * first data  segment, but is there a better way?  */
			dtls1_clear_record_buffer(s);

			s->init_msg = s->init_buf->data + DTLS1_HM_HEADER_LENGTH;
			return s->init_num;
			}
		else
			msg_hdr->frag_off = i;
		} while(1) ;

f_err:
	ssl3_send_alert(s,SSL3_AL_FATAL,al);
	*ok = 0;
	return -1;
	}