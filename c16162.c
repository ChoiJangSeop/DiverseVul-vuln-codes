static int get_server_verify(SSL *s)
	{
	unsigned char *p;
	int i, n, len;

	p=(unsigned char *)s->init_buf->data;
	if (s->state == SSL2_ST_GET_SERVER_VERIFY_A)
		{
		i=ssl2_read(s,(char *)&(p[s->init_num]),1-s->init_num);
		if (i < (1-s->init_num)) 
			return(ssl2_part_read(s,SSL_F_GET_SERVER_VERIFY,i));
		s->init_num += i;

		s->state= SSL2_ST_GET_SERVER_VERIFY_B;
		if (*p != SSL2_MT_SERVER_VERIFY)
			{
			if (p[0] != SSL2_MT_ERROR)
				{
				ssl2_return_error(s,SSL2_PE_UNDEFINED_ERROR);
				SSLerr(SSL_F_GET_SERVER_VERIFY,
					SSL_R_READ_WRONG_PACKET_TYPE);
				}
			else
				{
				SSLerr(SSL_F_GET_SERVER_VERIFY,SSL_R_PEER_ERROR);
				/* try to read the error message */
				i=ssl2_read(s,(char *)&(p[s->init_num]),3-s->init_num);
				return ssl2_part_read(s,SSL_F_GET_SERVER_VERIFY,i);
				}
			return(-1);
			}
		}
	
	p=(unsigned char *)s->init_buf->data;
	len = 1 + s->s2->challenge_length;
	n =  len - s->init_num;
	i = ssl2_read(s,(char *)&(p[s->init_num]),n);
	if (i < n)
		return(ssl2_part_read(s,SSL_F_GET_SERVER_VERIFY,i));
	if (s->msg_callback)
		s->msg_callback(0, s->version, 0, p, len, s, s->msg_callback_arg); /* SERVER-VERIFY */
	p += 1;

	if (memcmp(p,s->s2->challenge,s->s2->challenge_length) != 0)
		{
		ssl2_return_error(s,SSL2_PE_UNDEFINED_ERROR);
		SSLerr(SSL_F_GET_SERVER_VERIFY,SSL_R_CHALLENGE_IS_DIFFERENT);
		return(-1);
		}
	return(1);
	}