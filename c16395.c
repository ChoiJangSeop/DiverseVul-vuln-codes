int ssl3_client_hello(SSL *s)
	{
	unsigned char *buf;
	unsigned char *p,*d;
	int i;
	unsigned long Time,l;
#ifndef OPENSSL_NO_COMP
	int j;
	SSL_COMP *comp;
#endif

	buf=(unsigned char *)s->init_buf->data;
	if (s->state == SSL3_ST_CW_CLNT_HELLO_A)
		{
		if ((s->session == NULL) ||
			(s->session->ssl_version != s->version) ||
			(s->session->not_resumable))
			{
			if (!ssl_get_new_session(s,0))
				goto err;
			}
		/* else use the pre-loaded session */

		p=s->s3->client_random;
		Time=(unsigned long)time(NULL);			/* Time */
		l2n(Time,p);
		if (RAND_pseudo_bytes(p,SSL3_RANDOM_SIZE-4) <= 0)
			goto err;

		/* Do the message type and length last */
		d=p= &(buf[4]);

		*(p++)=s->version>>8;
		*(p++)=s->version&0xff;
		s->client_version=s->version;

		/* Random stuff */
		memcpy(p,s->s3->client_random,SSL3_RANDOM_SIZE);
		p+=SSL3_RANDOM_SIZE;

		/* Session ID */
		if (s->new_session)
			i=0;
		else
			i=s->session->session_id_length;
		*(p++)=i;
		if (i != 0)
			{
			if (i > (int)sizeof(s->session->session_id))
				{
				SSLerr(SSL_F_SSL3_CLIENT_HELLO, ERR_R_INTERNAL_ERROR);
				goto err;
				}
			memcpy(p,s->session->session_id,i);
			p+=i;
			}
		
		/* Ciphers supported */
		i=ssl_cipher_list_to_bytes(s,SSL_get_ciphers(s),&(p[2]),0);
		if (i == 0)
			{
			SSLerr(SSL_F_SSL3_CLIENT_HELLO,SSL_R_NO_CIPHERS_AVAILABLE);
			goto err;
			}
		s2n(i,p);
		p+=i;

		/* COMPRESSION */
#ifdef OPENSSL_NO_COMP
		*(p++)=1;
#else

		if ((s->options & SSL_OP_NO_COMPRESSION)
					|| !s->ctx->comp_methods)
			j=0;
		else
			j=sk_SSL_COMP_num(s->ctx->comp_methods);
		*(p++)=1+j;
		for (i=0; i<j; i++)
			{
			comp=sk_SSL_COMP_value(s->ctx->comp_methods,i);
			*(p++)=comp->id;
			}
#endif
		*(p++)=0; /* Add the NULL method */
#ifndef OPENSSL_NO_TLSEXT
		if ((p = ssl_add_clienthello_tlsext(s, p, buf+SSL3_RT_MAX_PLAIN_LENGTH)) == NULL)
			{
			SSLerr(SSL_F_SSL3_CLIENT_HELLO,ERR_R_INTERNAL_ERROR);
			goto err;
			}
#endif
		
		l=(p-d);
		d=buf;
		*(d++)=SSL3_MT_CLIENT_HELLO;
		l2n3(l,d);

		s->state=SSL3_ST_CW_CLNT_HELLO_B;
		/* number of bytes to write */
		s->init_num=p-buf;
		s->init_off=0;
		}

	/* SSL3_ST_CW_CLNT_HELLO_B */
	return(ssl3_do_write(s,SSL3_RT_HANDSHAKE));
err:
	return(-1);
	}