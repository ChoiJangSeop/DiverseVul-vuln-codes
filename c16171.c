int tls1_mac(SSL *ssl, unsigned char *md, int send)
	{
	SSL3_RECORD *rec;
	unsigned char *mac_sec,*seq;
	const EVP_MD *hash;
	unsigned int md_size;
	int i;
	HMAC_CTX hmac;
	unsigned char buf[5]; 

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

	buf[0]=rec->type;
	if (ssl->version == DTLS1_VERSION && ssl->client_version == DTLS1_BAD_VER)
		{
		buf[1]=TLS1_VERSION_MAJOR;
		buf[2]=TLS1_VERSION_MINOR;
		}
	else	{
		buf[1]=(unsigned char)(ssl->version>>8);
		buf[2]=(unsigned char)(ssl->version);
		}

	buf[3]=rec->length>>8;
	buf[4]=rec->length&0xff;

	/* I should fix this up TLS TLS TLS TLS TLS XXXXXXXX */
	HMAC_CTX_init(&hmac);
	HMAC_Init_ex(&hmac,mac_sec,EVP_MD_size(hash),hash,NULL);

	if (ssl->version == DTLS1_BAD_VER ||
	    (ssl->version == DTLS1_VERSION && ssl->client_version != DTLS1_BAD_VER))
		{
		unsigned char dtlsseq[8],*p=dtlsseq;
		s2n(send?ssl->d1->w_epoch:ssl->d1->r_epoch, p);
		memcpy (p,&seq[2],6);

		HMAC_Update(&hmac,dtlsseq,8);
		}
	else
		HMAC_Update(&hmac,seq,8);

	HMAC_Update(&hmac,buf,5);
	HMAC_Update(&hmac,rec->input,rec->length);
	HMAC_Final(&hmac,md,&md_size);
	HMAC_CTX_cleanup(&hmac);

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