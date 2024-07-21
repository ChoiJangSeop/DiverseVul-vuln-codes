SSL_SESSION *SSL_SESSION_new(void)
	{
	SSL_SESSION *ss;

	ss=(SSL_SESSION *)OPENSSL_malloc(sizeof(SSL_SESSION));
	if (ss == NULL)
		{
		SSLerr(SSL_F_SSL_SESSION_NEW,ERR_R_MALLOC_FAILURE);
		return(0);
		}
	memset(ss,0,sizeof(SSL_SESSION));

	ss->verify_result = 1; /* avoid 0 (= X509_V_OK) just in case */
	ss->references=1;
	ss->timeout=60*5+4; /* 5 minute timeout by default */
	ss->time=(unsigned long)time(NULL);
	ss->prev=NULL;
	ss->next=NULL;
	ss->compress_meth=0;
#ifndef OPENSSL_NO_TLSEXT
	ss->tlsext_hostname = NULL; 
#ifndef OPENSSL_NO_EC
	ss->tlsext_ecpointformatlist_length = 0;
	ss->tlsext_ecpointformatlist = NULL;
	ss->tlsext_ellipticcurvelist_length = 0;
	ss->tlsext_ellipticcurvelist = NULL;
#endif
#endif
	CRYPTO_new_ex_data(CRYPTO_EX_INDEX_SSL_SESSION, ss, &ss->ex_data);
#ifndef OPENSSL_NO_PSK
	ss->psk_identity_hint=NULL;
	ss->psk_identity=NULL;
#endif
	return(ss);
	}