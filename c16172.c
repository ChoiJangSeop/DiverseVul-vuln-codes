int tls1_enc(SSL *s, int send)
	{
	SSL3_RECORD *rec;
	EVP_CIPHER_CTX *ds;
	unsigned long l;
	int bs,i,ii,j,k;
	const EVP_CIPHER *enc;

	if (send)
		{
		ds=s->enc_write_ctx;
		rec= &(s->s3->wrec);
		if (s->enc_write_ctx == NULL)
			enc=NULL;
		else
			enc=EVP_CIPHER_CTX_cipher(s->enc_write_ctx);
		}
	else
		{
		ds=s->enc_read_ctx;
		rec= &(s->s3->rrec);
		if (s->enc_read_ctx == NULL)
			enc=NULL;
		else
			enc=EVP_CIPHER_CTX_cipher(s->enc_read_ctx);
		}

#ifdef KSSL_DEBUG
	printf("tls1_enc(%d)\n", send);
#endif    /* KSSL_DEBUG */

	if ((s->session == NULL) || (ds == NULL) ||
		(enc == NULL))
		{
		memmove(rec->data,rec->input,rec->length);
		rec->input=rec->data;
		}
	else
		{
		l=rec->length;
		bs=EVP_CIPHER_block_size(ds->cipher);

		if ((bs != 1) && send)
			{
			i=bs-((int)l%bs);

			/* Add weird padding of upto 256 bytes */

			/* we need to add 'i' padding bytes of value j */
			j=i-1;
			if (s->options & SSL_OP_TLS_BLOCK_PADDING_BUG)
				{
				if (s->s3->flags & TLS1_FLAGS_TLS_PADDING_BUG)
					j++;
				}
			for (k=(int)l; k<(int)(l+i); k++)
				rec->input[k]=j;
			l+=i;
			rec->length+=i;
			}

#ifdef KSSL_DEBUG
		{
                unsigned long ui;
		printf("EVP_Cipher(ds=%p,rec->data=%p,rec->input=%p,l=%ld) ==>\n",
                        (void *)ds,rec->data,rec->input,l);
		printf("\tEVP_CIPHER_CTX: %d buf_len, %d key_len [%ld %ld], %d iv_len\n",
                        ds->buf_len, ds->cipher->key_len,
                        (unsigned long)DES_KEY_SZ,
			(unsigned long)DES_SCHEDULE_SZ,
                        ds->cipher->iv_len);
		printf("\t\tIV: ");
		for (i=0; i<ds->cipher->iv_len; i++) printf("%02X", ds->iv[i]);
		printf("\n");
		printf("\trec->input=");
		for (ui=0; ui<l; ui++) printf(" %02x", rec->input[ui]);
		printf("\n");
		}
#endif	/* KSSL_DEBUG */

		if (!send)
			{
			if (l == 0 || l%bs != 0)
				{
				SSLerr(SSL_F_TLS1_ENC,SSL_R_BLOCK_CIPHER_PAD_IS_WRONG);
				ssl3_send_alert(s,SSL3_AL_FATAL,SSL_AD_DECRYPTION_FAILED);
				return 0;
				}
			}
		
		EVP_Cipher(ds,rec->data,rec->input,l);

#ifdef KSSL_DEBUG
		{
                unsigned long ki;
                printf("\trec->data=");
		for (ki=0; ki<l; i++)
                        printf(" %02x", rec->data[ki]);  printf("\n");
                }
#endif	/* KSSL_DEBUG */

		if ((bs != 1) && !send)
			{
			ii=i=rec->data[l-1]; /* padding_length */
			i++;
			/* NB: if compression is in operation the first packet
			 * may not be of even length so the padding bug check
			 * cannot be performed. This bug workaround has been
			 * around since SSLeay so hopefully it is either fixed
			 * now or no buggy implementation supports compression 
			 * [steve]
			 */
			if ( (s->options&SSL_OP_TLS_BLOCK_PADDING_BUG)
				&& !s->expand)
				{
				/* First packet is even in size, so check */
				if ((memcmp(s->s3->read_sequence,
					"\0\0\0\0\0\0\0\0",8) == 0) && !(ii & 1))
					s->s3->flags|=TLS1_FLAGS_TLS_PADDING_BUG;
				if (s->s3->flags & TLS1_FLAGS_TLS_PADDING_BUG)
					i--;
				}
			/* TLS 1.0 does not bound the number of padding bytes by the block size.
			 * All of them must have value 'padding_length'. */
			if (i > (int)rec->length)
				{
				/* Incorrect padding. SSLerr() and ssl3_alert are done
				 * by caller: we don't want to reveal whether this is
				 * a decryption error or a MAC verification failure
				 * (see http://www.openssl.org/~bodo/tls-cbc.txt) */
				return -1;
				}
			for (j=(int)(l-i); j<(int)l; j++)
				{
				if (rec->data[j] != ii)
					{
					/* Incorrect padding */
					return -1;
					}
				}
			rec->length-=i;
			}
		}
	return(1);
	}