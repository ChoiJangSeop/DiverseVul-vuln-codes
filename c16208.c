int tls1_mac(SSL *ssl, unsigned char *md, int send)
	{
	SSL3_RECORD *rec;
	unsigned char *seq;
	EVP_MD_CTX *hash;
	size_t md_size;
	int i;
	EVP_MD_CTX hmac, *mac_ctx;
	unsigned char buf[5]; 
	int stream_mac = (send?(ssl->mac_flags & SSL_MAC_FLAG_WRITE_MAC_STREAM):(ssl->mac_flags&SSL_MAC_FLAG_READ_MAC_STREAM));
	int t;

	if (send)
		{
		rec= &(ssl->s3->wrec);
		seq= &(ssl->s3->write_sequence[0]);
		hash=ssl->write_hash;
		}
	else
		{
		rec= &(ssl->s3->rrec);
		seq= &(ssl->s3->read_sequence[0]);
		hash=ssl->read_hash;
		}

	t=EVP_MD_CTX_size(hash);
	OPENSSL_assert(t >= 0);
	md_size=t;

	buf[0]=rec->type;
	buf[1]=(unsigned char)(ssl->version>>8);
	buf[2]=(unsigned char)(ssl->version);
	buf[3]=rec->length>>8;
	buf[4]=rec->length&0xff;

	/* I should fix this up TLS TLS TLS TLS TLS XXXXXXXX */
	if (stream_mac) 
		{
			mac_ctx = hash;
		}
		else
		{
			EVP_MD_CTX_copy(&hmac,hash);
			mac_ctx = &hmac;
		}

	if (ssl->version == DTLS1_VERSION || ssl->version == DTLS1_BAD_VER)
		{
		unsigned char dtlsseq[8],*p=dtlsseq;

		s2n(send?ssl->d1->w_epoch:ssl->d1->r_epoch, p);
		memcpy (p,&seq[2],6);

		EVP_DigestSignUpdate(mac_ctx,dtlsseq,8);
		}
	else
		EVP_DigestSignUpdate(mac_ctx,seq,8);

	EVP_DigestSignUpdate(mac_ctx,buf,5);
	EVP_DigestSignUpdate(mac_ctx,rec->input,rec->length);
	t=EVP_DigestSignFinal(mac_ctx,md,&md_size);
	OPENSSL_assert(t > 0);
		
	if (!stream_mac) EVP_MD_CTX_cleanup(&hmac);
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

	if (ssl->version != DTLS1_VERSION && ssl->version != DTLS1_BAD_VER)
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