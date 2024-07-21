int ssl3_enc(SSL *s, int send)
	{
	SSL3_RECORD *rec;
	EVP_CIPHER_CTX *ds;
	unsigned long l;
	int bs,i,mac_size=0;
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

		if (EVP_MD_CTX_md(s->read_hash) != NULL)
			mac_size = EVP_MD_CTX_size(s->read_hash);
		if ((bs != 1) && !send)
			return ssl3_cbc_remove_padding(s, rec, bs, mac_size);
		}
	return(1);
	}