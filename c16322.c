int ssl3_read_n(SSL *s, int n, int max, int extend)
	{
	/* If extend == 0, obtain new n-byte packet; if extend == 1, increase
	 * packet by another n bytes.
	 * The packet will be in the sub-array of s->s3->rbuf.buf specified
	 * by s->packet and s->packet_length.
	 * (If s->read_ahead is set, 'max' bytes may be stored in rbuf
	 * [plus s->packet_length bytes if extend == 1].)
	 */
	int i,len,left;
	long align=0;
	unsigned char *pkt;
	SSL3_BUFFER *rb;

	if (n <= 0) return n;

	rb    = &(s->s3->rbuf);
	left  = rb->left;
#if defined(SSL3_ALIGN_PAYLOAD) && SSL3_ALIGN_PAYLOAD!=0
	align = (long)rb->buf + SSL3_RT_HEADER_LENGTH;
	align = (-align)&(SSL3_ALIGN_PAYLOAD-1);
#endif

	if (!extend)
		{
		/* start with empty packet ... */
		if (left == 0)
			rb->offset = align;
		else if (align != 0 && left >= SSL3_RT_HEADER_LENGTH)
			{
			/* check if next packet length is large
			 * enough to justify payload alignment... */
			pkt = rb->buf + rb->offset;
			if (pkt[0] == SSL3_RT_APPLICATION_DATA
			    && (pkt[3]<<8|pkt[4]) >= 128)
				{
			 	/* Note that even if packet is corrupted
				 * and its length field is insane, we can
				 * only be led to wrong decision about
				 * whether memmove will occur or not.
				 * Header values has no effect on memmove
				 * arguments and therefore no buffer
				 * overrun can be triggered. */
				memmove (rb->buf+align,pkt,left);
				rb->offset = align;
				}
			}
		s->packet = rb->buf + rb->offset;
		s->packet_length = 0;
		/* ... now we can act as if 'extend' was set */
		}

	/* extend reads should not span multiple packets for DTLS */
	if ( SSL_version(s) == DTLS1_VERSION &&
		extend)
		{
		if ( left > 0 && n > left)
			n = left;
		}

	/* if there is enough in the buffer from a previous read, take some */
	if (left >= n)
		{
		s->packet_length+=n;
		rb->left=left-n;
		rb->offset+=n;
		return(n);
		}

	/* else we need to read more data */

	len = s->packet_length;
	pkt = rb->buf+align;
	/* Move any available bytes to front of buffer:
	 * 'len' bytes already pointed to by 'packet',
	 * 'left' extra ones at the end */
	if (s->packet != pkt) /* len > 0 */
		{
		memmove(pkt, s->packet, len+left);
		s->packet = pkt;
		rb->offset = len + align;
		}

	max = rb->len - rb->offset;
	if (n > max) /* does not happen */
		{
		SSLerr(SSL_F_SSL3_READ_N,ERR_R_INTERNAL_ERROR);
		return -1;
		}

	if (!s->read_ahead)
		max=n;

	while (left < n)
		{
		/* Now we have len+left bytes at the front of s->s3->rbuf.buf
		 * and need to read in more until we have len+n (up to
		 * len+max if possible) */

		clear_sys_error();
		if (s->rbio != NULL)
			{
			s->rwstate=SSL_READING;
			i=BIO_read(s->rbio,pkt+len+left, max-left);
			}
		else
			{
			SSLerr(SSL_F_SSL3_READ_N,SSL_R_READ_BIO_NOT_SET);
			i = -1;
			}

		if (i <= 0)
			{
			rb->left = left;
			return(i);
			}
		left+=i;
		}

	/* done reading, now the book-keeping */
	rb->offset += n;
	rb->left = left - n;
	s->packet_length += n;
	s->rwstate=SSL_NOTHING;
	return(n);
	}