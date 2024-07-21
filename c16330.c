int ssl3_write_pending(SSL *s, int type, const unsigned char *buf,
	unsigned int len)
	{
	int i;
	SSL3_BUFFER *wb=&(s->s3->wbuf);

/* XXXX */
	if ((s->s3->wpend_tot > (int)len)
		|| ((s->s3->wpend_buf != buf) &&
			!(s->mode & SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER))
		|| (s->s3->wpend_type != type))
		{
		SSLerr(SSL_F_SSL3_WRITE_PENDING,SSL_R_BAD_WRITE_RETRY);
		return(-1);
		}

	for (;;)
		{
		clear_sys_error();
		if (s->wbio != NULL)
			{
			s->rwstate=SSL_WRITING;
			i=BIO_write(s->wbio,
				(char *)&(wb->buf[wb->offset]),
				(unsigned int)wb->left);
			}
		else
			{
			SSLerr(SSL_F_SSL3_WRITE_PENDING,SSL_R_BIO_NOT_SET);
			i= -1;
			}
		if (i == wb->left)
			{
			wb->left=0;
			wb->offset+=i;
			if (s->mode & SSL_MODE_RELEASE_BUFFERS &&
				!SSL_IS_DTLS(s))
				ssl3_release_write_buffer(s);
			s->rwstate=SSL_NOTHING;
			return(s->s3->wpend_ret);
			}
		else if (i <= 0) {
			if (s->version == DTLS1_VERSION ||
			    s->version == DTLS1_BAD_VER) {
				/* For DTLS, just drop it. That's kind of the whole
				   point in using a datagram service */
				wb->left = 0;
			}
			return(i);
		}
		wb->offset+=i;
		wb->left-=i;
		}
	}