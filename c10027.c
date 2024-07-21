int context_init(SERVICE_OPTIONS *section) { /* init TLS context */
    /* create TLS context */
#if OPENSSL_VERSION_NUMBER>=0x10100000L
    if(section->option.client)
        section->ctx=SSL_CTX_new(TLS_client_method());
    else /* server mode */
        section->ctx=SSL_CTX_new(TLS_server_method());
    if(!SSL_CTX_set_min_proto_version(section->ctx,
            section->min_proto_version)) {
        s_log(LOG_ERR, "Failed to set the minimum protocol version 0x%X",
            section->min_proto_version);
        return 1; /* FAILED */
    }
    if(!SSL_CTX_set_max_proto_version(section->ctx,
            section->max_proto_version)) {
        s_log(LOG_ERR, "Failed to set the maximum protocol version 0x%X",
            section->max_proto_version);
        return 1; /* FAILED */
    }
#else /* OPENSSL_VERSION_NUMBER<0x10100000L */
    if(section->option.client)
        section->ctx=SSL_CTX_new(section->client_method);
    else /* server mode */
        section->ctx=SSL_CTX_new(section->server_method);
#endif /* OPENSSL_VERSION_NUMBER<0x10100000L */
    if(!section->ctx) {
        sslerror("SSL_CTX_new");
        return 1; /* FAILED */
    }
    /* for callbacks */
    if(!SSL_CTX_set_ex_data(section->ctx, index_ssl_ctx_opt, section)) {
        sslerror("SSL_CTX_set_ex_data");
        return 1; /* FAILED */
    }
    current_section=section; /* setup current section for callbacks */

    /* ciphers */
    if(section->cipher_list) {
        s_log(LOG_DEBUG, "Ciphers: %s", section->cipher_list);
        if(!SSL_CTX_set_cipher_list(section->ctx, section->cipher_list)) {
            sslerror("SSL_CTX_set_cipher_list");
            return 1; /* FAILED */
        }
    }

#ifndef OPENSSL_NO_TLS1_3
    /* ciphersuites */
    if(section->ciphersuites) {
        s_log(LOG_DEBUG, "TLSv1.3 ciphersuites: %s", section->ciphersuites);
        if(!SSL_CTX_set_ciphersuites(section->ctx, section->ciphersuites)) {
            sslerror("SSL_CTX_set_ciphersuites");
            return 1; /* FAILED */
        }
    }
#endif /* TLS 1.3 */

    /* TLS options: configure the stunnel defaults first */
    SSL_CTX_set_options(section->ctx, SSL_OP_NO_SSLv2|SSL_OP_NO_SSLv3);
#ifdef SSL_OP_NO_COMPRESSION
    /* we implemented a better way to disable compression if needed */
    SSL_CTX_clear_options(section->ctx, SSL_OP_NO_COMPRESSION);
#endif /* SSL_OP_NO_COMPRESSION */

    /* TLS options: configure the user-specified values */
    SSL_CTX_set_options(section->ctx,
        (SSL_OPTIONS_TYPE)(section->ssl_options_set));
#if OPENSSL_VERSION_NUMBER>=0x009080dfL
    SSL_CTX_clear_options(section->ctx,
        (SSL_OPTIONS_TYPE)(section->ssl_options_clear));
#endif /* OpenSSL 0.9.8m or later */

    /* TLS options: log the configured values */
#if OPENSSL_VERSION_NUMBER>=0x009080dfL
    s_log(LOG_DEBUG, "TLS options: 0x%08lX (+0x%08lX, -0x%08lX)",
        SSL_CTX_get_options(section->ctx),
        section->ssl_options_set, section->ssl_options_clear);
#else /* OpenSSL older than 0.9.8m */
    s_log(LOG_DEBUG, "TLS options: 0x%08lX (+0x%08lX)",
        SSL_CTX_get_options(section->ctx), section->ssl_options_set);
#endif /* OpenSSL 0.9.8m or later */

    /* initialize OpenSSL CONF options */
    if(conf_init(section))
        return 1; /* FAILED */

    /* mode */
#ifdef SSL_MODE_RELEASE_BUFFERS
    SSL_CTX_set_mode(section->ctx,
        SSL_MODE_ENABLE_PARTIAL_WRITE |
        SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER |
        SSL_MODE_RELEASE_BUFFERS);
#else
    SSL_CTX_set_mode(section->ctx,
        SSL_MODE_ENABLE_PARTIAL_WRITE |
        SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);
#endif

    /* setup session tickets */
#if OPENSSL_VERSION_NUMBER >= 0x10101000L
    SSL_CTX_set_session_ticket_cb(section->ctx, generate_session_ticket_cb,
        decrypt_session_ticket_cb, NULL);
#endif /* OpenSSL 1.1.1 or later */

#if OPENSSL_VERSION_NUMBER>=0x10000000L
    if((section->ticket_key)&&(section->ticket_mac))
        SSL_CTX_set_tlsext_ticket_key_cb(section->ctx, ssl_tlsext_ticket_key_cb);
#endif /* OpenSSL 1.0.0 or later */

    /* setup session cache */
    if(!section->option.client) {
        unsigned servname_len=(unsigned)strlen(section->servname);
        if(servname_len>SSL_MAX_SSL_SESSION_ID_LENGTH)
            servname_len=SSL_MAX_SSL_SESSION_ID_LENGTH;
        if(!SSL_CTX_set_session_id_context(section->ctx,
                (unsigned char *)section->servname, servname_len)) {
            sslerror("SSL_CTX_set_session_id_context");
            return 1; /* FAILED */
        }
    }
    SSL_CTX_set_session_cache_mode(section->ctx,
        SSL_SESS_CACHE_BOTH | SSL_SESS_CACHE_NO_INTERNAL_STORE);
    SSL_CTX_sess_set_cache_size(section->ctx, section->session_size);
    SSL_CTX_set_timeout(section->ctx, section->session_timeout);
    SSL_CTX_sess_set_new_cb(section->ctx, sess_new_cb);
    SSL_CTX_sess_set_get_cb(section->ctx, sess_get_cb);
    SSL_CTX_sess_set_remove_cb(section->ctx, sess_remove_cb);

    /* set info callback */
    SSL_CTX_set_info_callback(section->ctx, info_callback);

    /* load certificate and private key to be verified by the peer server */
    if(auth_init(section))
        return 1; /* FAILED */

    /* initialize verification of the peer server certificate */
    if(verify_init(section))
        return 1; /* FAILED */

    /* initialize the DH/ECDH key agreement in the server mode */
    if(!section->option.client) {
#ifndef OPENSSL_NO_TLSEXT
        SSL_CTX_set_tlsext_servername_callback(section->ctx, servername_cb);
#endif /* OPENSSL_NO_TLSEXT */
#ifndef OPENSSL_NO_DH
        dh_init(section); /* ignore the result (errors are not critical) */
#endif /* OPENSSL_NO_DH */
#ifndef OPENSSL_NO_ECDH
        ecdh_init(section); /* ignore the result (errors are not critical) */
#endif /* OPENSSL_NO_ECDH */
    }

    return 0; /* OK */
}