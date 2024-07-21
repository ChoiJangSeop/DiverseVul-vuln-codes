int ssl3_new(SSL *s)
	{
	SSL3_STATE *s3;

	if ((s3=OPENSSL_malloc(sizeof *s3)) == NULL) goto err;
	memset(s3,0,sizeof *s3);
	memset(s3->rrec.seq_num,0,sizeof(s3->rrec.seq_num));
	memset(s3->wrec.seq_num,0,sizeof(s3->wrec.seq_num));

	s->s3=s3;

	s->method->ssl_clear(s);
	return(1);
err:
	return(0);
	}