void SSL_SESSION_free(SSL_SESSION *ss)
	{
	int i;

	if(ss == NULL)
	    return;

	i=CRYPTO_add(&ss->references,-1,CRYPTO_LOCK_SSL_SESSION);
#ifdef REF_PRINT
	REF_PRINT("SSL_SESSION",ss);
#endif
	if (i > 0) return;
#ifdef REF_CHECK
	if (i < 0)
		{
		fprintf(stderr,"SSL_SESSION_free, bad reference count\n");
		abort(); /* ok */
		}
#endif

	CRYPTO_free_ex_data(CRYPTO_EX_INDEX_SSL_SESSION, ss, &ss->ex_data);

	OPENSSL_cleanse(ss->key_arg,sizeof ss->key_arg);
	OPENSSL_cleanse(ss->master_key,sizeof ss->master_key);
	OPENSSL_cleanse(ss->session_id,sizeof ss->session_id);
	if (ss->sess_cert != NULL) ssl_sess_cert_free(ss->sess_cert);
	if (ss->peer != NULL) X509_free(ss->peer);
	if (ss->ciphers != NULL) sk_SSL_CIPHER_free(ss->ciphers);
#ifndef OPENSSL_NO_TLSEXT
	if (ss->tlsext_hostname != NULL) OPENSSL_free(ss->tlsext_hostname);
	if (ss->tlsext_tick != NULL) OPENSSL_free(ss->tlsext_tick);
#ifndef OPENSSL_NO_EC
	ss->tlsext_ecpointformatlist_length = 0;
	if (ss->tlsext_ecpointformatlist != NULL) OPENSSL_free(ss->tlsext_ecpointformatlist);
	ss->tlsext_ellipticcurvelist_length = 0;
	if (ss->tlsext_ellipticcurvelist != NULL) OPENSSL_free(ss->tlsext_ellipticcurvelist);
#endif /* OPENSSL_NO_EC */
#endif
#ifndef OPENSSL_NO_PSK
	if (ss->psk_identity_hint != NULL)
		OPENSSL_free(ss->psk_identity_hint);
	if (ss->psk_identity != NULL)
		OPENSSL_free(ss->psk_identity);
#endif
	OPENSSL_cleanse(ss,sizeof(*ss));
	OPENSSL_free(ss);
	}