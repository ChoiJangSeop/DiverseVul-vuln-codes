dtls1_process_record(SSL *s)
{
    int al;
	int clear=0;
    int enc_err;
	SSL_SESSION *sess;
    SSL3_RECORD *rr;
	unsigned int mac_size;
	unsigned char md[EVP_MAX_MD_SIZE];
	int decryption_failed_or_bad_record_mac = 0;
	unsigned char *mac = NULL;
	int i;


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
	if (enc_err <= 0)
		{
		/* To minimize information leaked via timing, we will always
		 * perform all computations before discarding the message.
		 */
		decryption_failed_or_bad_record_mac = 1;
		}

#ifdef TLS_DEBUG
printf("dec %d\n",rr->length);
{ unsigned int z; for (z=0; z<rr->length; z++) printf("%02X%c",rr->data[z],((z+1)%16)?' ':'\n'); }
printf("\n");
#endif

	/* r->length is now the compressed data plus mac */
if (	(sess == NULL) ||
		(s->enc_read_ctx == NULL) ||
		(s->read_hash == NULL))
    clear=1;

	if (!clear)
		{
		mac_size=EVP_MD_size(s->read_hash);

		if (rr->length > SSL3_RT_MAX_COMPRESSED_LENGTH+mac_size)
			{
#if 0 /* OK only for stream ciphers (then rr->length is visible from ciphertext anyway) */
			al=SSL_AD_RECORD_OVERFLOW;
			SSLerr(SSL_F_DTLS1_PROCESS_RECORD,SSL_R_PRE_MAC_LENGTH_TOO_LONG);
			goto f_err;
#else
			decryption_failed_or_bad_record_mac = 1;
#endif			
			}
		/* check the MAC for rr->input (it's in mac_size bytes at the tail) */
		if (rr->length >= mac_size)
			{
			rr->length -= mac_size;
			mac = &rr->data[rr->length];
			}
		else
			rr->length = 0;
		i=s->method->ssl3_enc->mac(s,md,0);
		if (i < 0 || mac == NULL || CRYPTO_memcmp(md,mac,mac_size) != 0)
			{
			decryption_failed_or_bad_record_mac = 1;
			}
		}

	if (decryption_failed_or_bad_record_mac)
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
	/* So at this point the following is true
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