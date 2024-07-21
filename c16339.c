int ssl3_get_server_hello(SSL *s)
	{
	STACK_OF(SSL_CIPHER) *sk;
	SSL_CIPHER *c;
	unsigned char *p,*d;
	int i,al,ok;
	unsigned int j;
	long n;
#ifndef OPENSSL_NO_COMP
	SSL_COMP *comp;
#endif

	n=s->method->ssl_get_message(s,
		SSL3_ST_CR_SRVR_HELLO_A,
		SSL3_ST_CR_SRVR_HELLO_B,
		-1,
		20000, /* ?? */
		&ok);

	if (!ok) return((int)n);

	if ( SSL_version(s) == DTLS1_VERSION || SSL_version(s) == DTLS1_BAD_VER)
		{
		if ( s->s3->tmp.message_type == DTLS1_MT_HELLO_VERIFY_REQUEST)
			{
			if ( s->d1->send_cookie == 0)
				{
				s->s3->tmp.reuse_message = 1;
				return 1;
				}
			else /* already sent a cookie */
				{
				al=SSL_AD_UNEXPECTED_MESSAGE;
				SSLerr(SSL_F_SSL3_GET_SERVER_HELLO,SSL_R_BAD_MESSAGE_TYPE);
				goto f_err;
				}
			}
		}
	
	if ( s->s3->tmp.message_type != SSL3_MT_SERVER_HELLO)
		{
		al=SSL_AD_UNEXPECTED_MESSAGE;
		SSLerr(SSL_F_SSL3_GET_SERVER_HELLO,SSL_R_BAD_MESSAGE_TYPE);
		goto f_err;
		}

	d=p=(unsigned char *)s->init_msg;

	if ((p[0] != (s->version>>8)) || (p[1] != (s->version&0xff)))
		{
		SSLerr(SSL_F_SSL3_GET_SERVER_HELLO,SSL_R_WRONG_SSL_VERSION);
		s->version=(s->version&0xff00)|p[1];
		al=SSL_AD_PROTOCOL_VERSION;
		goto f_err;
		}
	p+=2;

	/* load the server hello data */
	/* load the server random */
	memcpy(s->s3->server_random,p,SSL3_RANDOM_SIZE);
	p+=SSL3_RANDOM_SIZE;

	/* get the session-id */
	j= *(p++);

	if ((j > sizeof s->session->session_id) || (j > SSL3_SESSION_ID_SIZE))
		{
		al=SSL_AD_ILLEGAL_PARAMETER;
		SSLerr(SSL_F_SSL3_GET_SERVER_HELLO,SSL_R_SSL3_SESSION_ID_TOO_LONG);
		goto f_err;
		}

	if (j != 0 && j == s->session->session_id_length
	    && memcmp(p,s->session->session_id,j) == 0)
	    {
	    if(s->sid_ctx_length != s->session->sid_ctx_length
	       || memcmp(s->session->sid_ctx,s->sid_ctx,s->sid_ctx_length))
		{
		/* actually a client application bug */
		al=SSL_AD_ILLEGAL_PARAMETER;
		SSLerr(SSL_F_SSL3_GET_SERVER_HELLO,SSL_R_ATTEMPT_TO_REUSE_SESSION_IN_DIFFERENT_CONTEXT);
		goto f_err;
		}
	    s->hit=1;
	    }
	else	/* a miss or crap from the other end */
		{
		/* If we were trying for session-id reuse, make a new
		 * SSL_SESSION so we don't stuff up other people */
		s->hit=0;
		if (s->session->session_id_length > 0)
			{
			if (!ssl_get_new_session(s,0))
				{
				al=SSL_AD_INTERNAL_ERROR;
				goto f_err;
				}
			}
		s->session->session_id_length=j;
		memcpy(s->session->session_id,p,j); /* j could be 0 */
		}
	p+=j;
	c=ssl_get_cipher_by_char(s,p);
	if (c == NULL)
		{
		/* unknown cipher */
		al=SSL_AD_ILLEGAL_PARAMETER;
		SSLerr(SSL_F_SSL3_GET_SERVER_HELLO,SSL_R_UNKNOWN_CIPHER_RETURNED);
		goto f_err;
		}
	p+=ssl_put_cipher_by_char(s,NULL,NULL);

	sk=ssl_get_ciphers_by_id(s);
	i=sk_SSL_CIPHER_find(sk,c);
	if (i < 0)
		{
		/* we did not say we would use this cipher */
		al=SSL_AD_ILLEGAL_PARAMETER;
		SSLerr(SSL_F_SSL3_GET_SERVER_HELLO,SSL_R_WRONG_CIPHER_RETURNED);
		goto f_err;
		}

	/* Depending on the session caching (internal/external), the cipher
	   and/or cipher_id values may not be set. Make sure that
	   cipher_id is set and use it for comparison. */
	if (s->session->cipher)
		s->session->cipher_id = s->session->cipher->id;
	if (s->hit && (s->session->cipher_id != c->id))
		{
/* Workaround is now obsolete */
#if 0
		if (!(s->options &
			SSL_OP_NETSCAPE_REUSE_CIPHER_CHANGE_BUG))
#endif
			{
			al=SSL_AD_ILLEGAL_PARAMETER;
			SSLerr(SSL_F_SSL3_GET_SERVER_HELLO,SSL_R_OLD_SESSION_CIPHER_NOT_RETURNED);
			goto f_err;
			}
		}
	s->s3->tmp.new_cipher=c;

	/* lets get the compression algorithm */
	/* COMPRESSION */
#ifdef OPENSSL_NO_COMP
	if (*(p++) != 0)
		{
		al=SSL_AD_ILLEGAL_PARAMETER;
		SSLerr(SSL_F_SSL3_GET_SERVER_HELLO,SSL_R_UNSUPPORTED_COMPRESSION_ALGORITHM);
		goto f_err;
		}
#else
	j= *(p++);
	if (j == 0)
		comp=NULL;
	else
		comp=ssl3_comp_find(s->ctx->comp_methods,j);
	
	if ((j != 0) && (comp == NULL))
		{
		al=SSL_AD_ILLEGAL_PARAMETER;
		SSLerr(SSL_F_SSL3_GET_SERVER_HELLO,SSL_R_UNSUPPORTED_COMPRESSION_ALGORITHM);
		goto f_err;
		}
	else
		{
		s->s3->tmp.new_compression=comp;
		}
#endif
#ifndef OPENSSL_NO_TLSEXT
	/* TLS extensions*/
	if (s->version >= SSL3_VERSION)
		{
		if (!ssl_parse_serverhello_tlsext(s,&p,d,n, &al))
			{
			/* 'al' set by ssl_parse_serverhello_tlsext */
			SSLerr(SSL_F_SSL3_GET_SERVER_HELLO,SSL_R_PARSE_TLSEXT);
			goto f_err; 
			}
		if (ssl_check_serverhello_tlsext(s) <= 0)
			{
			SSLerr(SSL_F_SSL3_GET_SERVER_HELLO,SSL_R_SERVERHELLO_TLSEXT);
				goto err;
			}
		}
#endif


	if (p != (d+n))
		{
		/* wrong packet length */
		al=SSL_AD_DECODE_ERROR;
		SSLerr(SSL_F_SSL3_GET_SERVER_HELLO,SSL_R_BAD_PACKET_LENGTH);
		goto f_err;
		}

	return(1);
f_err:
	ssl3_send_alert(s,SSL3_AL_FATAL,al);
#ifndef OPENSSL_NO_TLSEXT
err:
#endif
	return(-1);
	}