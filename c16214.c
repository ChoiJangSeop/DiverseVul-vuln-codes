int dtls1_enc(SSL *s, int send)
	{
	SSL3_RECORD *rec;
	EVP_CIPHER_CTX *ds;
	unsigned long l;
	int bs,i,ii,j,k,n=0;
	const EVP_CIPHER *enc;

	if (send)
		{
		if (EVP_MD_CTX_md(s->write_hash))
			{
			n=EVP_MD_CTX_size(s->write_hash);
			if (n < 0)
				return -1;
			}
		ds=s->enc_write_ctx;
		rec= &(s->s3->wrec);
		if (s->enc_write_ctx == NULL)
			enc=NULL;
		else
			{
			enc=EVP_CIPHER_CTX_cipher(s->enc_write_ctx);
			if ( rec->data != rec->input)
				/* we can't write into the input stream */
				fprintf(stderr, "%s:%d: rec->data != rec->input\n",
					__FILE__, __LINE__);
			else if ( EVP_CIPHER_block_size(ds->cipher) > 1)
				{
				if (RAND_bytes(rec->input, EVP_CIPHER_block_size(ds->cipher)) <= 0)
					return -1;
				}
			}
		}
	else
		{
		if (EVP_MD_CTX_md(s->read_hash))
			{
			n=EVP_MD_CTX_size(s->read_hash);
			if (n < 0)
				return -1;
			}
		ds=s->enc_read_ctx;
		rec= &(s->s3->rrec);
		if (s->enc_read_ctx == NULL)
			enc=NULL;
		else
			enc=EVP_CIPHER_CTX_cipher(s->enc_read_ctx);
		}

#ifdef KSSL_DEBUG
	printf("dtls1_enc(%d)\n", send);
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
                        ds,rec->data,rec->input,l);
		printf("\tEVP_CIPHER_CTX: %d buf_len, %d key_len [%d %d], %d iv_len\n",
                        ds->buf_len, ds->cipher->key_len,
                        DES_KEY_SZ, DES_SCHEDULE_SZ,
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
				return -1;
			}
		
		EVP_Cipher(ds,rec->data,rec->input,l);

#ifdef KSSL_DEBUG
		{
                unsigned long i;
                printf("\trec->data=");
		for (i=0; i<l; i++)
                        printf(" %02x", rec->data[i]);  printf("\n");
                }
#endif	/* KSSL_DEBUG */

		if ((bs != 1) && !send)
			{
			ii=i=rec->data[l-1]; /* padding_length */
			i++;
			if (s->options&SSL_OP_TLS_BLOCK_PADDING_BUG)
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
			if (i + bs > (int)rec->length)
				{
				/* Incorrect padding. SSLerr() and ssl3_alert are done
				 * by caller: we don't want to reveal whether this is
				 * a decryption error or a MAC verification failure
				 * (see http://www.openssl.org/~bodo/tls-cbc.txt) 
				 */
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

			rec->data += bs;    /* skip the implicit IV */
			rec->input += bs;
			rec->length -= bs;
			}
		}
	return(1);
	}