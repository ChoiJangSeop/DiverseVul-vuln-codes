static int ssl2_read_internal(SSL *s, void *buf, int len, int peek)
	{
	int n;
	unsigned char mac[MAX_MAC_SIZE];
	unsigned char *p;
	int i;
	int mac_size;

 ssl2_read_again:
	if (SSL_in_init(s) && !s->in_handshake)
		{
		n=s->handshake_func(s);
		if (n < 0) return(n);
		if (n == 0)
			{
			SSLerr(SSL_F_SSL2_READ_INTERNAL,SSL_R_SSL_HANDSHAKE_FAILURE);
			return(-1);
			}
		}

	clear_sys_error();
	s->rwstate=SSL_NOTHING;
	if (len <= 0) return(len);

	if (s->s2->ract_data_length != 0) /* read from buffer */
		{
		if (len > s->s2->ract_data_length)
			n=s->s2->ract_data_length;
		else
			n=len;

		memcpy(buf,s->s2->ract_data,(unsigned int)n);
		if (!peek)
			{
			s->s2->ract_data_length-=n;
			s->s2->ract_data+=n;
			if (s->s2->ract_data_length == 0)
				s->rstate=SSL_ST_READ_HEADER;
			}

		return(n);
		}

	/* s->s2->ract_data_length == 0
	 * 
	 * Fill the buffer, then goto ssl2_read_again.
	 */

	if (s->rstate == SSL_ST_READ_HEADER)
		{
		if (s->first_packet)
			{
			n=read_n(s,5,SSL2_MAX_RECORD_LENGTH_2_BYTE_HEADER+2,0);
			if (n <= 0) return(n); /* error or non-blocking */
			s->first_packet=0;
			p=s->packet;
			if (!((p[0] & 0x80) && (
				(p[2] == SSL2_MT_CLIENT_HELLO) ||
				(p[2] == SSL2_MT_SERVER_HELLO))))
				{
				SSLerr(SSL_F_SSL2_READ_INTERNAL,SSL_R_NON_SSLV2_INITIAL_PACKET);
				return(-1);
				}
			}
		else
			{
			n=read_n(s,2,SSL2_MAX_RECORD_LENGTH_2_BYTE_HEADER+2,0);
			if (n <= 0) return(n); /* error or non-blocking */
			}
		/* part read stuff */

		s->rstate=SSL_ST_READ_BODY;
		p=s->packet;
		/* Do header */
		/*s->s2->padding=0;*/
		s->s2->escape=0;
		s->s2->rlength=(((unsigned int)p[0])<<8)|((unsigned int)p[1]);
		if ((p[0] & TWO_BYTE_BIT))		/* Two byte header? */
			{
			s->s2->three_byte_header=0;
			s->s2->rlength&=TWO_BYTE_MASK;	
			}
		else
			{
			s->s2->three_byte_header=1;
			s->s2->rlength&=THREE_BYTE_MASK;

			/* security >s2->escape */
			s->s2->escape=((p[0] & SEC_ESC_BIT))?1:0;
			}
		}

	if (s->rstate == SSL_ST_READ_BODY)
		{
		n=s->s2->rlength+2+s->s2->three_byte_header;
		if (n > (int)s->packet_length)
			{
			n-=s->packet_length;
			i=read_n(s,(unsigned int)n,(unsigned int)n,1);
			if (i <= 0) return(i); /* ERROR */
			}

		p= &(s->packet[2]);
		s->rstate=SSL_ST_READ_HEADER;
		if (s->s2->three_byte_header)
			s->s2->padding= *(p++);
		else	s->s2->padding=0;

		/* Data portion */
		if (s->s2->clear_text)
			{
			mac_size = 0;
			s->s2->mac_data=p;
			s->s2->ract_data=p;
			if (s->s2->padding)
				{
				SSLerr(SSL_F_SSL2_READ_INTERNAL,SSL_R_ILLEGAL_PADDING);
				return(-1);
				}
			}
		else
			{
			mac_size=EVP_MD_CTX_size(s->read_hash);
			if (mac_size < 0)
				return -1;
			OPENSSL_assert(mac_size <= MAX_MAC_SIZE);
			s->s2->mac_data=p;
			s->s2->ract_data= &p[mac_size];
			if (s->s2->padding + mac_size > s->s2->rlength)
				{
				SSLerr(SSL_F_SSL2_READ_INTERNAL,SSL_R_ILLEGAL_PADDING);
				return(-1);
				}
			}

		s->s2->ract_data_length=s->s2->rlength;
		/* added a check for length > max_size in case
		 * encryption was not turned on yet due to an error */
		if ((!s->s2->clear_text) &&
			(s->s2->rlength >= (unsigned int)mac_size))
			{
			ssl2_enc(s,0);
			s->s2->ract_data_length-=mac_size;
			ssl2_mac(s,mac,0);
			s->s2->ract_data_length-=s->s2->padding;
			if (	(memcmp(mac,s->s2->mac_data,
				(unsigned int)mac_size) != 0) ||
				(s->s2->rlength%EVP_CIPHER_CTX_block_size(s->enc_read_ctx) != 0))
				{
				SSLerr(SSL_F_SSL2_READ_INTERNAL,SSL_R_BAD_MAC_DECODE);
				return(-1);
				}
			}
		INC32(s->s2->read_sequence); /* expect next number */
		/* s->s2->ract_data is now available for processing */

		/* Possibly the packet that we just read had 0 actual data bytes.
		 * (SSLeay/OpenSSL itself never sends such packets; see ssl2_write.)
		 * In this case, returning 0 would be interpreted by the caller
		 * as indicating EOF, so it's not a good idea.  Instead, we just
		 * continue reading; thus ssl2_read_internal may have to process
		 * multiple packets before it can return.
		 *
		 * [Note that using select() for blocking sockets *never* guarantees
		 * that the next SSL_read will not block -- the available
		 * data may contain incomplete packets, and except for SSL 2,
		 * renegotiation can confuse things even more.] */

		goto ssl2_read_again; /* This should really be
		                       * "return ssl2_read(s,buf,len)",
		                       * but that would allow for
		                       * denial-of-service attacks if a
		                       * C compiler is used that does not
		                       * recognize end-recursion. */
		}
	else
		{
		SSLerr(SSL_F_SSL2_READ_INTERNAL,SSL_R_BAD_STATE);
			return(-1);
		}
	}