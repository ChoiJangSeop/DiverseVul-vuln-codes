void ssl3_free(SSL *s)
	{
	if(s == NULL)
	    return;

#ifdef TLSEXT_TYPE_opaque_prf_input
	if (s->s3->client_opaque_prf_input != NULL)
		OPENSSL_free(s->s3->client_opaque_prf_input);
	if (s->s3->server_opaque_prf_input != NULL)
		OPENSSL_free(s->s3->server_opaque_prf_input);
#endif

	ssl3_cleanup_key_block(s);
	if (s->s3->rbuf.buf != NULL)
		ssl3_release_read_buffer(s);
	if (s->s3->wbuf.buf != NULL)
		ssl3_release_write_buffer(s);
	if (s->s3->rrec.comp != NULL)
		OPENSSL_free(s->s3->rrec.comp);
#ifndef OPENSSL_NO_DH
	if (s->s3->tmp.dh != NULL)
		DH_free(s->s3->tmp.dh);
#endif
#ifndef OPENSSL_NO_ECDH
	if (s->s3->tmp.ecdh != NULL)
		EC_KEY_free(s->s3->tmp.ecdh);
#endif

	if (s->s3->tmp.ca_names != NULL)
		sk_X509_NAME_pop_free(s->s3->tmp.ca_names,X509_NAME_free);
	if (s->s3->handshake_buffer) {
		BIO_free(s->s3->handshake_buffer);
	}
	if (s->s3->handshake_dgst) ssl3_free_digest_list(s);
	OPENSSL_cleanse(s->s3,sizeof *s->s3);
	OPENSSL_free(s->s3);
	s->s3=NULL;
	}