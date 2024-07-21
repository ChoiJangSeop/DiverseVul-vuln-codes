void SSL_free(SSL *s)
	{
	int i;

	if(s == NULL)
	    return;

	i=CRYPTO_add(&s->references,-1,CRYPTO_LOCK_SSL);
#ifdef REF_PRINT
	REF_PRINT("SSL",s);
#endif
	if (i > 0) return;
#ifdef REF_CHECK
	if (i < 0)
		{
		fprintf(stderr,"SSL_free, bad reference count\n");
		abort(); /* ok */
		}
#endif

	if (s->param)
		X509_VERIFY_PARAM_free(s->param);

	CRYPTO_free_ex_data(CRYPTO_EX_INDEX_SSL, s, &s->ex_data);

	if (s->bbio != NULL)
		{
		/* If the buffering BIO is in place, pop it off */
		if (s->bbio == s->wbio)
			{
			s->wbio=BIO_pop(s->wbio);
			}
		BIO_free(s->bbio);
		s->bbio=NULL;
		}
	if (s->rbio != NULL)
		BIO_free_all(s->rbio);
	if ((s->wbio != NULL) && (s->wbio != s->rbio))
		BIO_free_all(s->wbio);

	if (s->init_buf != NULL) BUF_MEM_free(s->init_buf);

	/* add extra stuff */
	if (s->cipher_list != NULL) sk_SSL_CIPHER_free(s->cipher_list);
	if (s->cipher_list_by_id != NULL) sk_SSL_CIPHER_free(s->cipher_list_by_id);

	/* Make the next call work :-) */
	if (s->session != NULL)
		{
		ssl_clear_bad_session(s);
		SSL_SESSION_free(s->session);
		}

	ssl_clear_cipher_ctx(s);
	ssl_clear_hash_ctx(&s->read_hash);
	ssl_clear_hash_ctx(&s->write_hash);

	if (s->cert != NULL) ssl_cert_free(s->cert);
	/* Free up if allocated */

#ifndef OPENSSL_NO_TLSEXT
	if (s->tlsext_hostname)
		OPENSSL_free(s->tlsext_hostname);
	if (s->initial_ctx) SSL_CTX_free(s->initial_ctx);
#ifndef OPENSSL_NO_EC
	if (s->tlsext_ecpointformatlist) OPENSSL_free(s->tlsext_ecpointformatlist);
	if (s->tlsext_ellipticcurvelist) OPENSSL_free(s->tlsext_ellipticcurvelist);
#endif /* OPENSSL_NO_EC */
	if (s->tlsext_opaque_prf_input) OPENSSL_free(s->tlsext_opaque_prf_input);
	if (s->tlsext_ocsp_exts)
		sk_X509_EXTENSION_pop_free(s->tlsext_ocsp_exts,
						X509_EXTENSION_free);
	if (s->tlsext_ocsp_ids)
		sk_OCSP_RESPID_pop_free(s->tlsext_ocsp_ids, OCSP_RESPID_free);
	if (s->tlsext_ocsp_resp)
		OPENSSL_free(s->tlsext_ocsp_resp);
#endif

	if (s->client_CA != NULL)
		sk_X509_NAME_pop_free(s->client_CA,X509_NAME_free);

	if (s->method != NULL) s->method->ssl_free(s);

	if (s->ctx) SSL_CTX_free(s->ctx);

#ifndef	OPENSSL_NO_KRB5
	if (s->kssl_ctx != NULL)
		kssl_ctx_free(s->kssl_ctx);
#endif	/* OPENSSL_NO_KRB5 */

	OPENSSL_free(s);
	}