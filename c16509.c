dtls1_process_record(SSL *s)
{
	int i,al;
	int enc_err;
	SSL_SESSION *sess;
	SSL3_RECORD *rr;
	unsigned int mac_size;
	unsigned char md[EVP_MAX_MD_SIZE];

	rr= &(s->s3->rrec);
	sess = s->session;

	/* At this point, s->packet_length == SSL3_RT_HEADER_LNGTH + rr->length,
	 * and we have that many bytes in s->packet
	 */
	rr->input= &(s->packet[DTLS1_RT_HEADER_LENGTH]);

	/* ok, we can now read from 's->packet' data into 'rr'
	 * rr->input points at rr->length bytes, which
	 * need to be copied into rr->data by either
	 * the decryption or by the decompression
	 * When the data is 'copied' into the rr->data buffer,
	 * rr->input will be pointed at the new buffer */ 

	/* We now have - encrypted [ MAC [ compressed [ plain ] ] ]
	 * rr->length bytes of encrypted compressed stuff. */

	/* check is not needed I believe */
	if (rr->length > SSL3_RT_MAX_ENCRYPTED_LENGTH)
		{
		al=SSL_AD_RECORD_OVERFLOW;
		SSLerr(SSL_F_DTLS1_PROCESS_RECORD,SSL_R_ENCRYPTED_LENGTH_TOO_LONG);
		goto f_err;
		}

	/* decrypt in place in 'rr->input' */
	rr->data=rr->input;
	rr->orig_len=rr->length;

	enc_err = s->method->ssl3_enc->enc(s,0);
	/* enc_err is:
	 *    0: (in non-constant time) if the record is publically invalid.
	 *    1: if the padding is valid
	 *    -1: if the padding is invalid */
	if (enc_err == 0)
		{
		/* For DTLS we simply ignore bad packets. */
		rr->length = 0;
		s->packet_length = 0;
		goto err;
		}

#ifdef TLS_DEBUG
printf("dec %d\n",rr->length);
{ unsigned int z; for (z=0; z<rr->length; z++) printf("%02X%c",rr->data[z],((z+1)%16)?' ':'\n'); }
printf("\n");
#endif

	/* r->length is now the compressed data plus mac */
	if ((sess != NULL) &&
	    (s->enc_read_ctx != NULL) &&
	    (EVP_MD_CTX_md(s->read_hash) != NULL))
		{
		/* s->read_hash != NULL => mac_size != -1 */
		unsigned char *mac = NULL;
		unsigned char mac_tmp[EVP_MAX_MD_SIZE];
		mac_size=EVP_MD_CTX_size(s->read_hash);
		OPENSSL_assert(mac_size <= EVP_MAX_MD_SIZE);

		/* orig_len is the length of the record before any padding was
		 * removed. This is public information, as is the MAC in use,
		 * therefore we can safely process the record in a different
		 * amount of time if it's too short to possibly contain a MAC.
		 */
		if (rr->orig_len < mac_size ||
		    /* CBC records must have a padding length byte too. */
		    (EVP_CIPHER_CTX_mode(s->enc_read_ctx) == EVP_CIPH_CBC_MODE &&
		     rr->orig_len < mac_size+1))
			{
			al=SSL_AD_DECODE_ERROR;
			SSLerr(SSL_F_DTLS1_PROCESS_RECORD,SSL_R_LENGTH_TOO_SHORT);
			goto f_err;
			}

		if (EVP_CIPHER_CTX_mode(s->enc_read_ctx) == EVP_CIPH_CBC_MODE)
			{
			/* We update the length so that the TLS header bytes
			 * can be constructed correctly but we need to extract
			 * the MAC in constant time from within the record,
			 * without leaking the contents of the padding bytes.
			 * */
			mac = mac_tmp;
			ssl3_cbc_copy_mac(mac_tmp, rr, mac_size);
			rr->length -= mac_size;
			}
		else
			{
			/* In this case there's no padding, so |rec->orig_len|
			 * equals |rec->length| and we checked that there's
			 * enough bytes for |mac_size| above. */
			rr->length -= mac_size;
			mac = &rr->data[rr->length];
			}

		i=s->method->ssl3_enc->mac(s,md,0 /* not send */);
		if (i < 0 || mac == NULL || CRYPTO_memcmp(md, mac, (size_t)mac_size) != 0)
			enc_err = -1;
		if (rr->length > SSL3_RT_MAX_COMPRESSED_LENGTH+mac_size)
			enc_err = -1;
		}

	if (enc_err < 0)
		{
		/* decryption failed, silently discard message */
		rr->length = 0;
		s->packet_length = 0;
		goto err;
		}

	/* r->length is now just compressed */
	if (s->expand != NULL)
		{
		if (rr->length > SSL3_RT_MAX_COMPRESSED_LENGTH)
			{
			al=SSL_AD_RECORD_OVERFLOW;
			SSLerr(SSL_F_DTLS1_PROCESS_RECORD,SSL_R_COMPRESSED_LENGTH_TOO_LONG);
			goto f_err;
			}
		if (!ssl3_do_uncompress(s))
			{
			al=SSL_AD_DECOMPRESSION_FAILURE;
			SSLerr(SSL_F_DTLS1_PROCESS_RECORD,SSL_R_BAD_DECOMPRESSION);
			goto f_err;
			}
		}

	if (rr->length > SSL3_RT_MAX_PLAIN_LENGTH)
		{
		al=SSL_AD_RECORD_OVERFLOW;
		SSLerr(SSL_F_DTLS1_PROCESS_RECORD,SSL_R_DATA_LENGTH_TOO_LONG);
		goto f_err;
		}

	rr->off=0;
	/*-
	 * So at this point the following is true
	 * ssl->s3->rrec.type 	is the type of record
	 * ssl->s3->rrec.length	== number of bytes in record
	 * ssl->s3->rrec.off	== offset to first valid byte
	 * ssl->s3->rrec.data	== where to take bytes from, increment
	 *			   after use :-).
	 */

	/* we have pulled in a full packet so zero things */
	s->packet_length=0;
	dtls1_record_bitmap_update(s, &(s->d1->bitmap));/* Mark receipt of record. */
	return(1);

f_err:
	ssl3_send_alert(s,SSL3_AL_FATAL,al);
err:
	return(0);
}