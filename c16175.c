int tls1_mac(SSL *ssl, unsigned char *md, int send)
	{
	SSL3_RECORD *rec;
	unsigned char *mac_sec,*seq;
	const EVP_MD *hash;
	unsigned int md_size;
	int i;
	EVP_MD_CTX hmac, *mac_ctx;
	unsigned char header[13];
	int stream_mac = (send?(ssl->mac_flags & SSL_MAC_FLAG_WRITE_MAC_STREAM):(ssl->mac_flags&SSL_MAC_FLAG_READ_MAC_STREAM));
	int t;

	if (send)
		{
		rec= &(ssl->s3->wrec);
		mac_sec= &(ssl->s3->write_mac_secret[0]);
		seq= &(ssl->s3->write_sequence[0]);
		hash=ssl->write_hash;
		}
	else
		{
		rec= &(ssl->s3->rrec);
		mac_sec= &(ssl->s3->read_mac_secret[0]);
		seq= &(ssl->s3->read_sequence[0]);
		hash=ssl->read_hash;
		}

	md_size=EVP_MD_size(hash);

	/* I should fix this up TLS TLS TLS TLS TLS XXXXXXXX */
	HMAC_CTX_init(&hmac);
	HMAC_Init_ex(&hmac,mac_sec,EVP_MD_size(hash),hash,NULL);

	if (ssl->version == DTLS1_BAD_VER ||
	    (ssl->version == DTLS1_VERSION && ssl->client_version != DTLS1_BAD_VER))
		{
		unsigned char dtlsseq[8],*p=dtlsseq;
		s2n(send?ssl->d1->w_epoch:ssl->d1->r_epoch, p);
		memcpy (p,&seq[2],6);

		memcpy(header, dtlsseq, 8);
		}
	else
		memcpy(header, seq, 8);

	header[8]=rec->type;
	header[9]=(unsigned char)(ssl->version>>8);
	header[10]=(unsigned char)(ssl->version);
	header[11]=(rec->length)>>8;
	header[12]=(rec->length)&0xff;

	if (!send &&
	    EVP_CIPHER_CTX_mode(ssl->enc_read_ctx) == EVP_CIPH_CBC_MODE &&
	    ssl3_cbc_record_digest_supported(mac_ctx))
		{
		/* This is a CBC-encrypted record. We must avoid leaking any
		 * timing-side channel information about how many blocks of
		 * data we are hashing because that gives an attacker a
		 * timing-oracle. */
		ssl3_cbc_digest_record(
			mac_ctx,
			md, &md_size,
			header, rec->input,
			rec->length + md_size, rec->orig_len,
			ssl->s3->read_mac_secret,
			ssl->s3->read_mac_secret_size,
			0 /* not SSLv3 */);
		}
	else
		{
		EVP_DigestSignUpdate(mac_ctx,header,sizeof(header));
		EVP_DigestSignUpdate(mac_ctx,rec->input,rec->length);
		t=EVP_DigestSignFinal(mac_ctx,md,&md_size);
		OPENSSL_assert(t > 0);
		}
		
	if (!stream_mac)
		EVP_MD_CTX_cleanup(&hmac);
#ifdef TLS_DEBUG
printf("sec=");
{unsigned int z; for (z=0; z<md_size; z++) printf("%02X ",mac_sec[z]); printf("\n"); }
printf("seq=");
{int z; for (z=0; z<8; z++) printf("%02X ",seq[z]); printf("\n"); }
printf("buf=");
{int z; for (z=0; z<5; z++) printf("%02X ",buf[z]); printf("\n"); }
printf("rec=");
{unsigned int z; for (z=0; z<rec->length; z++) printf("%02X ",buf[z]); printf("\n"); }
#endif

	if ( SSL_version(ssl) != DTLS1_VERSION && SSL_version(ssl) != DTLS1_BAD_VER)
		{
		for (i=7; i>=0; i--)
			{
			++seq[i];
			if (seq[i] != 0) break; 
			}
		}

#ifdef TLS_DEBUG
{unsigned int z; for (z=0; z<md_size; z++) printf("%02X ",md[z]); printf("\n"); }
#endif
	return(md_size);
	}