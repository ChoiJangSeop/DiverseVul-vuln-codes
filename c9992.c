static int reds_init_ssl(RedsState *reds)
{
    const SSL_METHOD *ssl_method;
    int return_code;
    /* Limit connection to TLSv1.1 or newer.
     * When some other SSL/TLS version becomes obsolete, add it to this
     * variable. */
    long ssl_options = SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_COMPRESSION | SSL_OP_NO_TLSv1;

    /* Global system initialization*/
    openssl_global_init();

    /* Create our context*/
    /* SSLv23_method() handles TLSv1.x in addition to SSLv2/v3 */
    ssl_method = SSLv23_method();
    reds->ctx = SSL_CTX_new(ssl_method);
    if (!reds->ctx) {
        spice_warning("Could not allocate new SSL context");
        red_dump_openssl_errors();
        return -1;
    }

    SSL_CTX_set_options(reds->ctx, ssl_options);
#if HAVE_DECL_SSL_CTX_SET_ECDH_AUTO || defined(SSL_CTX_set_ecdh_auto)
    SSL_CTX_set_ecdh_auto(reds->ctx, 1);
#endif

    /* Load our keys and certificates*/
    return_code = SSL_CTX_use_certificate_chain_file(reds->ctx, reds->config->ssl_parameters.certs_file);
    if (return_code == 1) {
        spice_debug("Loaded certificates from %s", reds->config->ssl_parameters.certs_file);
    } else {
        spice_warning("Could not load certificates from %s", reds->config->ssl_parameters.certs_file);
        red_dump_openssl_errors();
        return -1;
    }

    SSL_CTX_set_default_passwd_cb(reds->ctx, ssl_password_cb);
    SSL_CTX_set_default_passwd_cb_userdata(reds->ctx, reds);

    return_code = SSL_CTX_use_PrivateKey_file(reds->ctx, reds->config->ssl_parameters.private_key_file,
                                              SSL_FILETYPE_PEM);
    if (return_code == 1) {
        spice_debug("Using private key from %s", reds->config->ssl_parameters.private_key_file);
    } else {
        spice_warning("Could not use private key file");
        return -1;
    }

    /* Load the CAs we trust*/
    return_code = SSL_CTX_load_verify_locations(reds->ctx, reds->config->ssl_parameters.ca_certificate_file, 0);
    if (return_code == 1) {
        spice_debug("Loaded CA certificates from %s", reds->config->ssl_parameters.ca_certificate_file);
    } else {
        spice_warning("Could not use CA file %s", reds->config->ssl_parameters.ca_certificate_file);
        red_dump_openssl_errors();
        return -1;
    }

    if (strlen(reds->config->ssl_parameters.dh_key_file) > 0) {
        if (load_dh_params(reds->ctx, reds->config->ssl_parameters.dh_key_file) < 0) {
            return -1;
        }
    }

    SSL_CTX_set_session_id_context(reds->ctx, (const unsigned char *)"SPICE", 5);
    if (strlen(reds->config->ssl_parameters.ciphersuite) > 0) {
        if (!SSL_CTX_set_cipher_list(reds->ctx, reds->config->ssl_parameters.ciphersuite)) {
            return -1;
        }
    }

    return 0;
}