SSL *SSL_new(SSL_CTX *ctx)
	{
	SSL *s;

	if (ctx == NULL)
		{
		SSLerr(SSL_F_SSL_NEW,SSL_R_NULL_SSL_CTX);
		return(NULL);
		}
	if (ctx->method == NULL)
		{
		SSLerr(SSL_F_SSL_NEW,SSL_R_SSL_CTX_HAS_NO_DEFAULT_SSL_VERSION);
		return(NULL);
		}

	s=(SSL *)OPENSSL_malloc(sizeof(SSL));
	if (s == NULL) goto err;
	memset(s,0,sizeof(SSL));

#ifndef	OPENSSL_NO_KRB5
	s->kssl_ctx = kssl_ctx_new();
#endif	/* OPENSSL_NO_KRB5 */

	s->options=ctx->options;
	s->mode=ctx->mode;
	s->max_cert_list=ctx->max_cert_list;

	if (ctx->cert != NULL)
		{
		/* Earlier library versions used to copy the pointer to
		 * the CERT, not its contents; only when setting new
		 * parameters for the per-SSL copy, ssl_cert_new would be
		 * called (and the direct reference to the per-SSL_CTX
		 * settings would be lost, but those still were indirectly
		 * accessed for various purposes, and for that reason they
		 * used to be known as s->ctx->default_cert).
		 * Now we don't look at the SSL_CTX's CERT after having
		 * duplicated it once. */

		s->cert = ssl_cert_dup(ctx->cert);
		if (s->cert == NULL)
			goto err;
		}
	else
		s->cert=NULL; /* Cannot really happen (see SSL_CTX_new) */

	s->read_ahead=ctx->read_ahead;
	s->msg_callback=ctx->msg_callback;
	s->msg_callback_arg=ctx->msg_callback_arg;
	s->verify_mode=ctx->verify_mode;
#if 0
	s->verify_depth=ctx->verify_depth;
#endif
	s->sid_ctx_length=ctx->sid_ctx_length;
	OPENSSL_assert(s->sid_ctx_length <= sizeof s->sid_ctx);
	memcpy(&s->sid_ctx,&ctx->sid_ctx,sizeof(s->sid_ctx));
	s->verify_callback=ctx->default_verify_callback;
	s->generate_session_id=ctx->generate_session_id;

	s->param = X509_VERIFY_PARAM_new();
	if (!s->param)
		goto err;
	X509_VERIFY_PARAM_inherit(s->param, ctx->param);
#if 0
	s->purpose = ctx->purpose;
	s->trust = ctx->trust;
#endif
	s->quiet_shutdown=ctx->quiet_shutdown;
	s->max_send_fragment = ctx->max_send_fragment;

	CRYPTO_add(&ctx->references,1,CRYPTO_LOCK_SSL_CTX);
	s->ctx=ctx;
#ifndef OPENSSL_NO_TLSEXT
	s->tlsext_debug_cb = 0;
	s->tlsext_debug_arg = NULL;
	s->tlsext_ticket_expected = 0;
	s->tlsext_status_type = -1;
	s->tlsext_status_expected = 0;
	s->tlsext_ocsp_ids = NULL;
	s->tlsext_ocsp_exts = NULL;
	s->tlsext_ocsp_resp = NULL;
	s->tlsext_ocsp_resplen = -1;
	CRYPTO_add(&ctx->references,1,CRYPTO_LOCK_SSL_CTX);
	s->initial_ctx=ctx;
#endif

	s->verify_result=X509_V_OK;

	s->method=ctx->method;

	if (!s->method->ssl_new(s))
		goto err;

	s->references=1;
	s->server=(ctx->method->ssl_accept == ssl_undefined_function)?0:1;

	SSL_clear(s);

	CRYPTO_new_ex_data(CRYPTO_EX_INDEX_SSL, s, &s->ex_data);

#ifndef OPENSSL_NO_PSK
	s->psk_client_callback=ctx->psk_client_callback;
	s->psk_server_callback=ctx->psk_server_callback;
#endif

	return(s);
err:
	if (s != NULL)
		{
		if (s->cert != NULL)
			ssl_cert_free(s->cert);
		if (s->ctx != NULL)
			SSL_CTX_free(s->ctx); /* decrement reference count */
		OPENSSL_free(s);
		}
	SSLerr(SSL_F_SSL_NEW,ERR_R_MALLOC_FAILURE);
	return(NULL);
	}