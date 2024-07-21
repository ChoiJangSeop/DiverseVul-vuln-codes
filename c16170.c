int ssl3_enc(SSL *s, int send)
	{
	SSL3_RECORD *rec;
	EVP_CIPHER_CTX *ds;
	unsigned long l;
	int bs,i;
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

		/* COMPRESS */

		if ((bs != 1) && send)
			{
			i=bs-((int)l%bs);

			/* we need to add 'i-1' padding bytes */
			l+=i;
			/* the last of these zero bytes will be overwritten
			 * with the padding length. */
			memset(&rec->input[rec->length], 0, i);
			rec->length+=i;
			rec->input[l-1]=(i-1);
			}
		
		if (!send)
			{
			if (l == 0 || l%bs != 0)
				{
				SSLerr(SSL_F_SSL3_ENC,SSL_R_BLOCK_CIPHER_PAD_IS_WRONG);
				ssl3_send_alert(s,SSL3_AL_FATAL,SSL_AD_DECRYPTION_FAILED);
				return 0;
				}
			/* otherwise, rec->length >= bs */
			}
		
		EVP_Cipher(ds,rec->data,rec->input,l);

		if ((bs != 1) && !send)
			{
			i=rec->data[l-1]+1;
			/* SSL 3.0 bounds the number of padding bytes by the block size;
			 * padding bytes (except the last one) are arbitrary */
			if (i > bs)
				{
				/* Incorrect padding. SSLerr() and ssl3_alert are done
				 * by caller: we don't want to reveal whether this is
				 * a decryption error or a MAC verification failure
				 * (see http://www.openssl.org/~bodo/tls-cbc.txt) */
				return -1;
				}
			/* now i <= bs <= rec->length */
			rec->length-=i;
			}
		}
	return(1);
	}