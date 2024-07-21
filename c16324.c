int ssl3_setup_buffers(SSL *s)
	{
	unsigned char *p;
	size_t len,align=0;

#if defined(SSL3_ALIGN_PAYLOAD) && SSL3_ALIGN_PAYLOAD!=0
	align = (-SSL3_RT_HEADER_LENGTH)&(SSL3_ALIGN_PAYLOAD-1);
#endif

	if (s->s3->rbuf.buf == NULL)
		{
		len = SSL3_RT_MAX_PLAIN_LENGTH
			+ SSL3_RT_MAX_ENCRYPTED_OVERHEAD
			+ SSL3_RT_HEADER_LENGTH + align;
		if (s->options & SSL_OP_MICROSOFT_BIG_SSLV3_BUFFER)
			{
			s->s3->init_extra = 1;
			len += SSL3_RT_MAX_EXTRA;
			}
#ifndef OPENSSL_NO_COMP
		if (!(s->options & SSL_OP_NO_COMPRESSION))
			len += SSL3_RT_MAX_COMPRESSED_OVERHEAD;
#endif
		if ((p=OPENSSL_malloc(len)) == NULL)
			goto err;
		s->s3->rbuf.buf = p;
		s->s3->rbuf.len = len;
		}

	if (s->s3->wbuf.buf == NULL)
		{
		len = s->max_send_fragment
			+ SSL3_RT_SEND_MAX_ENCRYPTED_OVERHEAD
			+ SSL3_RT_HEADER_LENGTH + align;
#ifndef OPENSSL_NO_COMP
		if (!(s->options & SSL_OP_NO_COMPRESSION))
			len += SSL3_RT_MAX_COMPRESSED_OVERHEAD;
#endif
		if (!(s->options & SSL_OP_DONT_INSERT_EMPTY_FRAGMENTS))
			len += SSL3_RT_HEADER_LENGTH + align
				+ SSL3_RT_SEND_MAX_ENCRYPTED_OVERHEAD;
		if ((p=OPENSSL_malloc(len)) == NULL)
			goto err;
		s->s3->wbuf.buf = p;
		s->s3->wbuf.len = len;
		}
	s->packet= &(s->s3->rbuf.buf[0]);
	return(1);
err:
	SSLerr(SSL_F_SSL3_SETUP_BUFFERS,ERR_R_MALLOC_FAILURE);
	return(0);
	}