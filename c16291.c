long ssl3_get_message(SSL *s, int st1, int stn, int mt, long max, int *ok)
	{
	unsigned char *p;
	unsigned long l;
	long n;
	int i,al;

	if (s->s3->tmp.reuse_message)
		{
		s->s3->tmp.reuse_message=0;
		if ((mt >= 0) && (s->s3->tmp.message_type != mt))
			{
			al=SSL_AD_UNEXPECTED_MESSAGE;
			SSLerr(SSL_F_SSL3_GET_MESSAGE,SSL_R_UNEXPECTED_MESSAGE);
			goto f_err;
			}
		*ok=1;
		s->init_msg = s->init_buf->data + 4;
		s->init_num = (int)s->s3->tmp.message_size;
		return s->init_num;
		}

	p=(unsigned char *)s->init_buf->data;

	if (s->state == st1) /* s->init_num < 4 */
		{
		int skip_message;

		do
			{
			while (s->init_num < 4)
				{
				i=s->method->ssl_read_bytes(s,SSL3_RT_HANDSHAKE,
					&p[s->init_num],4 - s->init_num, 0);
				if (i <= 0)
					{
					s->rwstate=SSL_READING;
					*ok = 0;
					return i;
					}
				s->init_num+=i;
				}
			
			skip_message = 0;
			if (!s->server)
				if (p[0] == SSL3_MT_HELLO_REQUEST)
					/* The server may always send 'Hello Request' messages --
					 * we are doing a handshake anyway now, so ignore them
					 * if their format is correct. Does not count for
					 * 'Finished' MAC. */
					if (p[1] == 0 && p[2] == 0 &&p[3] == 0)
						{
						s->init_num = 0;
						skip_message = 1;

						if (s->msg_callback)
							s->msg_callback(0, s->version, SSL3_RT_HANDSHAKE, p, 4, s, s->msg_callback_arg);
						}
			}
		while (skip_message);

		/* s->init_num == 4 */

		if ((mt >= 0) && (*p != mt))
			{
			al=SSL_AD_UNEXPECTED_MESSAGE;
			SSLerr(SSL_F_SSL3_GET_MESSAGE,SSL_R_UNEXPECTED_MESSAGE);
			goto f_err;
			}
		if ((mt < 0) && (*p == SSL3_MT_CLIENT_HELLO) &&
					(st1 == SSL3_ST_SR_CERT_A) &&
					(stn == SSL3_ST_SR_CERT_B))
			{
			/* At this point we have got an MS SGC second client
			 * hello (maybe we should always allow the client to
			 * start a new handshake?). We need to restart the mac.
			 * Don't increment {num,total}_renegotiations because
			 * we have not completed the handshake. */
			ssl3_init_finished_mac(s);
			}

		s->s3->tmp.message_type= *(p++);

		n2l3(p,l);
		if (l > (unsigned long)max)
			{
			al=SSL_AD_ILLEGAL_PARAMETER;
			SSLerr(SSL_F_SSL3_GET_MESSAGE,SSL_R_EXCESSIVE_MESSAGE_SIZE);
			goto f_err;
			}
		if (l > (INT_MAX-4)) /* BUF_MEM_grow takes an 'int' parameter */
			{
			al=SSL_AD_ILLEGAL_PARAMETER;
			SSLerr(SSL_F_SSL3_GET_MESSAGE,SSL_R_EXCESSIVE_MESSAGE_SIZE);
			goto f_err;
			}
		if (l && !BUF_MEM_grow_clean(s->init_buf,(int)l+4))
			{
			SSLerr(SSL_F_SSL3_GET_MESSAGE,ERR_R_BUF_LIB);
			goto err;
			}
		s->s3->tmp.message_size=l;
		s->state=stn;

		s->init_msg = s->init_buf->data + 4;
		s->init_num = 0;
		}

	/* next state (stn) */
	p = s->init_msg;
	n = s->s3->tmp.message_size - s->init_num;
	while (n > 0)
		{
		i=s->method->ssl_read_bytes(s,SSL3_RT_HANDSHAKE,&p[s->init_num],n,0);
		if (i <= 0)
			{
			s->rwstate=SSL_READING;
			*ok = 0;
			return i;
			}
		s->init_num += i;
		n -= i;
		}
	ssl3_finish_mac(s, (unsigned char *)s->init_buf->data, s->init_num + 4);
	if (s->msg_callback)
		s->msg_callback(0, s->version, SSL3_RT_HANDSHAKE, s->init_buf->data, (size_t)s->init_num + 4, s, s->msg_callback_arg);
	*ok=1;
	return s->init_num;
f_err:
	ssl3_send_alert(s,SSL3_AL_FATAL,al);
err:
	*ok=0;
	return(-1);
	}