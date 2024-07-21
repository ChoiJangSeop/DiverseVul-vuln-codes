int n_ssl3_mac(SSL *ssl, unsigned char *md, int send)
	{
	SSL3_RECORD *rec;
	unsigned char *mac_sec,*seq;
	EVP_MD_CTX md_ctx;
	const EVP_MD_CTX *hash;
	unsigned char *p,rec_char;
	unsigned int md_size;
	int npad;
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

	t=EVP_MD_CTX_size(hash);
	if (t < 0)
		return -1;
	md_size=t;
	npad=(48/md_size)*md_size;

	/* Chop the digest off the end :-) */
	EVP_MD_CTX_init(&md_ctx);

	EVP_MD_CTX_copy_ex( &md_ctx,hash);
	EVP_DigestUpdate(&md_ctx,mac_sec,md_size);
	EVP_DigestUpdate(&md_ctx,ssl3_pad_1,npad);
	EVP_DigestUpdate(&md_ctx,seq,8);
	rec_char=rec->type;
	EVP_DigestUpdate(&md_ctx,&rec_char,1);
	p=md;
	s2n(rec->length,p);
	EVP_DigestUpdate(&md_ctx,md,2);
	EVP_DigestUpdate(&md_ctx,rec->input,rec->length);
	EVP_DigestFinal_ex( &md_ctx,md,NULL);

	EVP_MD_CTX_copy_ex( &md_ctx,hash);
	EVP_DigestUpdate(&md_ctx,mac_sec,md_size);
	EVP_DigestUpdate(&md_ctx,ssl3_pad_2,npad);
	EVP_DigestUpdate(&md_ctx,md,md_size);
	EVP_DigestFinal_ex( &md_ctx,md,&md_size);

	EVP_MD_CTX_cleanup(&md_ctx);

	ssl3_record_sequence_update(seq);
	return(md_size);
	}