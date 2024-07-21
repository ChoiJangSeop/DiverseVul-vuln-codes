void ssl_cert_free(CERT *c)
	{
	int i;

	if(c == NULL)
	    return;

	i=CRYPTO_add(&c->references,-1,CRYPTO_LOCK_SSL_CERT);
#ifdef REF_PRINT
	REF_PRINT("CERT",c);
#endif
	if (i > 0) return;
#ifdef REF_CHECK
	if (i < 0)
		{
		fprintf(stderr,"ssl_cert_free, bad reference count\n");
		abort(); /* ok */
		}
#endif

#ifndef OPENSSL_NO_RSA
	if (c->rsa_tmp) RSA_free(c->rsa_tmp);
#endif
#ifndef OPENSSL_NO_DH
	if (c->dh_tmp) DH_free(c->dh_tmp);
#endif
#ifndef OPENSSL_NO_ECDH
	if (c->ecdh_tmp) EC_KEY_free(c->ecdh_tmp);
#endif

	ssl_cert_clear_certs(c);
	if (c->peer_sigalgs)
		OPENSSL_free(c->peer_sigalgs);
	if (c->conf_sigalgs)
		OPENSSL_free(c->conf_sigalgs);
	OPENSSL_free(c);
	}