static int gg_session_init_ssl(struct gg_session *gs)
{
#ifdef GG_CONFIG_HAVE_GNUTLS
	gg_session_gnutls_t *tmp;

	tmp = (gg_session_gnutls_t*) gs->ssl;

	if (tmp == NULL) {
		tmp = malloc(sizeof(gg_session_gnutls_t));

		if (tmp == NULL) {
			gg_debug(GG_DEBUG_MISC, "// gg_session_connect() out of memory for GnuTLS session\n");
			return -1;
		}

		memset(tmp, 0, sizeof(gg_session_gnutls_t));

		gs->ssl = tmp;

		gnutls_global_init();
		gnutls_certificate_allocate_credentials(&tmp->xcred);
	} else {
		gnutls_deinit(tmp->session);
	}

	gnutls_init(&tmp->session, GNUTLS_CLIENT);
	gnutls_set_default_priority(tmp->session);
	gnutls_credentials_set(tmp->session, GNUTLS_CRD_CERTIFICATE, tmp->xcred);
	gnutls_transport_set_ptr(tmp->session, (gnutls_transport_ptr_t) (long) gs->fd);
#endif

#ifdef GG_CONFIG_HAVE_OPENSSL
	char buf[1024];

	OpenSSL_add_ssl_algorithms();

	if (!RAND_status()) {
		char rdata[1024];
		struct {
			time_t time;
			void *ptr;
		} rstruct;

		time(&rstruct.time);
		rstruct.ptr = (void *) &rstruct;

		RAND_seed((void *) rdata, sizeof(rdata));
		RAND_seed((void *) &rstruct, sizeof(rstruct));
	}

	if (gs->ssl_ctx == NULL) {
		gs->ssl_ctx = SSL_CTX_new(SSLv3_client_method());

		if (gs->ssl_ctx == NULL) {
			ERR_error_string_n(ERR_get_error(), buf, sizeof(buf));
			gg_debug(GG_DEBUG_MISC, "// gg_session_connect() SSL_CTX_new() failed: %s\n", buf);
			return -1;
		}

		SSL_CTX_set_verify(gs->ssl_ctx, SSL_VERIFY_NONE, NULL);
		SSL_CTX_set_default_verify_paths(gs->ssl_ctx);
	}

	if (gs->ssl != NULL)
		SSL_free(gs->ssl);

	gs->ssl = SSL_new(gs->ssl_ctx);

	if (gs->ssl == NULL) {
		ERR_error_string_n(ERR_get_error(), buf, sizeof(buf));
		gg_debug(GG_DEBUG_MISC, "// gg_session_connect() SSL_new() failed: %s\n", buf);
		return -1;
	}

	SSL_set_fd(gs->ssl, gs->fd);
#endif

	return 0;
}