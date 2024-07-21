static pj_status_t init_openssl(void)
{
    pj_status_t status;

    if (openssl_init_count)
	return PJ_SUCCESS;

    openssl_init_count = 1;

    /* Register error subsystem */
    status = pj_register_strerror(PJ_SSL_ERRNO_START, 
				  PJ_SSL_ERRNO_SPACE_SIZE, 
				  &ssl_strerror);
    pj_assert(status == PJ_SUCCESS);

    /* Init OpenSSL lib */
#if USING_LIBRESSL || OPENSSL_VERSION_NUMBER < 0x10100000L
    SSL_library_init();
    SSL_load_error_strings();
#else
    OPENSSL_init_ssl(0, NULL);
#endif
#if OPENSSL_VERSION_NUMBER < 0x009080ffL
    /* This is now synonym of SSL_library_init() */
    OpenSSL_add_all_algorithms();
#endif

    /* Init available ciphers */
    if (ssl_cipher_num == 0 || ssl_curves_num == 0) {
	SSL_METHOD *meth = NULL;
	SSL_CTX *ctx;
	SSL *ssl;
	STACK_OF(SSL_CIPHER) *sk_cipher;
	SSL_SESSION *ssl_sess;
	unsigned i, n;
	int nid;
	const char *cname;

#if (USING_LIBRESSL && LIBRESSL_VERSION_NUMBER < 0x2020100fL)\
    || OPENSSL_VERSION_NUMBER < 0x10100000L

	meth = (SSL_METHOD*)SSLv23_server_method();
	if (!meth)
	    meth = (SSL_METHOD*)TLSv1_server_method();
#ifndef OPENSSL_NO_SSL3_METHOD
	if (!meth)
	    meth = (SSL_METHOD*)SSLv3_server_method();
#endif
#ifndef OPENSSL_NO_SSL2
	if (!meth)
	    meth = (SSL_METHOD*)SSLv2_server_method();
#endif

#else
	/* Specific version methods are deprecated in 1.1.0 */
	meth = (SSL_METHOD*)TLS_method();
#endif

	pj_assert(meth);

	ctx=SSL_CTX_new(meth);
	SSL_CTX_set_cipher_list(ctx, "ALL:COMPLEMENTOFALL");

	ssl = SSL_new(ctx);

	sk_cipher = SSL_get_ciphers(ssl);

	n = sk_SSL_CIPHER_num(sk_cipher);
	if (n > PJ_ARRAY_SIZE(ssl_ciphers) - ADDITIONAL_CIPHER_COUNT)
	    n = PJ_ARRAY_SIZE(ssl_ciphers) - ADDITIONAL_CIPHER_COUNT;

	for (i = 0; i < n; ++i) {
	    const SSL_CIPHER *c;
	    c = sk_SSL_CIPHER_value(sk_cipher,i);
	    ssl_ciphers[i].id = (pj_ssl_cipher)
				    (pj_uint32_t)SSL_CIPHER_get_id(c) &
				    0x00FFFFFF;
	    ssl_ciphers[i].name = SSL_CIPHER_get_name(c);
	}

	/* Add cipher aliases not returned from SSL_get_ciphers() */
	for (i = 0; i < ADDITIONAL_CIPHER_COUNT; ++i) {
	    ssl_ciphers[n++] = ADDITIONAL_CIPHERS[i];
	}
	ssl_cipher_num = n;

	ssl_sess = SSL_SESSION_new();
	SSL_set_session(ssl, ssl_sess);

#if !USING_LIBRESSL && !defined(OPENSSL_NO_EC) \
    && OPENSSL_VERSION_NUMBER >= 0x1000200fL
#if OPENSSL_VERSION_NUMBER >= 0x1010100fL
	ssl_curves_num = EC_get_builtin_curves(NULL, 0);
#else
	ssl_curves_num = SSL_get_shared_curve(ssl,-1);

	if (ssl_curves_num > PJ_ARRAY_SIZE(ssl_curves))
	    ssl_curves_num = PJ_ARRAY_SIZE(ssl_curves);
#endif

	if( ssl_curves_num > 0 ) {
#if OPENSSL_VERSION_NUMBER >= 0x1010100fL
	    EC_builtin_curve * curves = NULL;

	    curves = OPENSSL_malloc((int)sizeof(*curves) * ssl_curves_num);
	    if (!EC_get_builtin_curves(curves, ssl_curves_num)) {
		OPENSSL_free(curves);
		curves = NULL;
		ssl_curves_num = 0;
	    }

	    n = ssl_curves_num;
	    ssl_curves_num = 0;

	    for (i = 0; i < n; i++) {
		nid = curves[i].nid;

		if ( 0 != get_cid_from_nid(nid) ) {
		    cname = OBJ_nid2sn(nid);

		    if (!cname)
			cname = OBJ_nid2sn(nid);

		    if (cname) {
			ssl_curves[ssl_curves_num].id = get_cid_from_nid(nid);
			ssl_curves[ssl_curves_num].name = cname;

			ssl_curves_num++;

			if (ssl_curves_num >= PJ_SSL_SOCK_MAX_CURVES )
			    break;
		    }
		}
	    }

	    if(curves)
		OPENSSL_free(curves);
#else
	for (i = 0; i < ssl_curves_num; i++) {
	    nid = SSL_get_shared_curve(ssl, i);

	    if (nid & TLSEXT_nid_unknown) {
		cname = "curve unknown";
		nid &= 0xFFFF;
	    } else {
		cname = EC_curve_nid2nist(nid);
		if (!cname)
		    cname = OBJ_nid2sn(nid);
	    }

	    ssl_curves[i].id   = get_cid_from_nid(nid);
	    ssl_curves[i].name = cname;
	}
#endif

	}
#else
	PJ_UNUSED_ARG(nid);
	PJ_UNUSED_ARG(cname);
	ssl_curves_num = 0;
#endif

	SSL_free(ssl);

	/* On OpenSSL 1.1.1, omitting SSL_SESSION_free() will cause 
	 * memory leak (e.g: as reported by Address Sanitizer). But using
	 * SSL_SESSION_free() may cause crash (due to double free?) on 1.0.x.
	 * As OpenSSL docs specifies to not calling SSL_SESSION_free() after
	 * SSL_free(), perhaps it is safer to obey this, the leak amount seems
	 * to be relatively small (<500 bytes) and should occur once only in
	 * the library lifetime.
#if OPENSSL_VERSION_NUMBER >= 0x10101000L
	SSL_SESSION_free(ssl_sess);
#endif
	 */

	SSL_CTX_free(ctx);
    }

    /* Create OpenSSL application data index for SSL socket */
    sslsock_idx = SSL_get_ex_new_index(0, "SSL socket", NULL, NULL, NULL);

#if defined(PJ_SSL_SOCK_OSSL_USE_THREAD_CB) && \
    PJ_SSL_SOCK_OSSL_USE_THREAD_CB != 0 && OPENSSL_VERSION_NUMBER < 0x10100000L

    status = init_ossl_lock();
    if (status != PJ_SUCCESS)
        return status;
#endif

    return status;
}