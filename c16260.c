int tls1_final_finish_mac(SSL *s,
	     const char *str, int slen, unsigned char *out)
	{
	unsigned int i;
	EVP_MD_CTX ctx;
	unsigned char buf[2*EVP_MAX_MD_SIZE];
	unsigned char *q,buf2[12];
	int idx;
	long mask;
	int err=0;
	const EVP_MD *md; 

	q=buf;

	if (s->s3->handshake_buffer) 
		if (!ssl3_digest_cached_records(s))
			return 0;

	EVP_MD_CTX_init(&ctx);

	for (idx=0;ssl_get_handshake_digest(idx,&mask,&md);idx++)
		{
		if (mask & ssl_get_algorithm2(s))
			{
			int hashsize = EVP_MD_size(md);
			if (hashsize < 0 || hashsize > (int)(sizeof buf - (size_t)(q-buf)))
				{
				/* internal error: 'buf' is too small for this cipersuite! */
				err = 1;
				}
			else
				{
				EVP_MD_CTX_copy_ex(&ctx,s->s3->handshake_dgst[idx]);
				EVP_DigestFinal_ex(&ctx,q,&i);
				if (i != (unsigned int)hashsize) /* can't really happen */
					err = 1;
				q+=i;
				}
			}
		}
		
	if (!tls1_PRF(ssl_get_algorithm2(s),
			str,slen, buf,(int)(q-buf), NULL,0, NULL,0, NULL,0,
			s->session->master_key,s->session->master_key_length,
			out,buf2,sizeof buf2))
		err = 1;
	EVP_MD_CTX_cleanup(&ctx);

	if (err)
		return 0;
	else
		return sizeof buf2;
	}