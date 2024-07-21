int ssl3_send_finished(SSL *s, int a, int b, const char *sender, int slen)
	{
	unsigned char *p,*d;
	int i;
	unsigned long l;

	if (s->state == a)
		{
		d=(unsigned char *)s->init_buf->data;
		p= &(d[4]);

		i=s->method->ssl3_enc->final_finish_mac(s,
			sender,slen,s->s3->tmp.finish_md);
		s->s3->tmp.finish_md_len = i;
		memcpy(p, s->s3->tmp.finish_md, i);
		p+=i;
		l=i;

                /* Copy the finished so we can use it for
                   renegotiation checks */
                if(s->type == SSL_ST_CONNECT)
                        {
                         OPENSSL_assert(i <= EVP_MAX_MD_SIZE);
                         memcpy(s->s3->previous_client_finished, 
                             s->s3->tmp.finish_md, i);
                         s->s3->previous_client_finished_len=i;
                        }
                else
                        {
                        OPENSSL_assert(i <= EVP_MAX_MD_SIZE);
                        memcpy(s->s3->previous_server_finished, 
                            s->s3->tmp.finish_md, i);
                        s->s3->previous_server_finished_len=i;
                        }

#ifdef OPENSSL_SYS_WIN16
		/* MSVC 1.5 does not clear the top bytes of the word unless
		 * I do this.
		 */
		l&=0xffff;
#endif

		*(d++)=SSL3_MT_FINISHED;
		l2n3(l,d);
		s->init_num=(int)l+4;
		s->init_off=0;

		s->state=b;
		}

	/* SSL3_ST_SEND_xxxxxx_HELLO_B */
	return(ssl3_do_write(s,SSL3_RT_HANDSHAKE));
	}