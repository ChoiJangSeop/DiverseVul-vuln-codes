void SSL_CTX_free(SSL_CTX *a)
	{
	int i;

	if (a == NULL) return;

	i=CRYPTO_add(&a->references,-1,CRYPTO_LOCK_SSL_CTX);
#ifdef REF_PRINT
	REF_PRINT("SSL_CTX",a);
#endif
	if (i > 0) return;
#ifdef REF_CHECK
	if (i < 0)
		{
		fprintf(stderr,"SSL_CTX_free, bad reference count\n");
		abort(); /* ok */
		}
#endif

	if (a->param)
		X509_VERIFY_PARAM_free(a->param);

	/*
	 * Free internal session cache. However: the remove_cb() may reference
	 * the ex_data of SSL_CTX, thus the ex_data store can only be removed
	 * after the sessions were flushed.
	 * As the ex_data handling routines might also touch the session cache,
	 * the most secure solution seems to be: empty (flush) the cache, then
	 * free ex_data, then finally free the cache.
	 * (See ticket [openssl.org #212].)
	 */
	if (a->sessions != NULL)
		SSL_CTX_flush_sessions(a,0);

	CRYPTO_free_ex_data(CRYPTO_EX_INDEX_SSL_CTX, a, &a->ex_data);

	if (a->sessions != NULL)
		lh_SSL_SESSION_free(a->sessions);

	if (a->cert_store != NULL)
		X509_STORE_free(a->cert_store);
	if (a->cipher_list != NULL)
		sk_SSL_CIPHER_free(a->cipher_list);
	if (a->cipher_list_by_id != NULL)
		sk_SSL_CIPHER_free(a->cipher_list_by_id);
	if (a->cert != NULL)
		ssl_cert_free(a->cert);
	if (a->client_CA != NULL)
		sk_X509_NAME_pop_free(a->client_CA,X509_NAME_free);
	if (a->extra_certs != NULL)
		sk_X509_pop_free(a->extra_certs,X509_free);
#if 0 /* This should never be done, since it removes a global database */
	if (a->comp_methods != NULL)
		sk_SSL_COMP_pop_free(a->comp_methods,SSL_COMP_free);
#else
	a->comp_methods = NULL;
#endif

#ifndef OPENSSL_NO_PSK
	if (a->psk_identity_hint)
		OPENSSL_free(a->psk_identity_hint);
#endif
#ifndef OPENSSL_NO_ENGINE
	if (a->client_cert_engine)
		ENGINE_finish(a->client_cert_engine);
#endif
	OPENSSL_free(a);
	}