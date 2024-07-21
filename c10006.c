NOEXPORT void print_cipher(CLI *c) { /* print negotiated cipher */
    SSL_CIPHER *cipher;
#ifndef OPENSSL_NO_COMP
    const COMP_METHOD *compression, *expansion;
#endif

    if(c->opt->log_level<LOG_INFO) /* performance optimization */
        return;

    s_log(LOG_INFO, "TLS %s: %s",
        c->opt->option.client ? "connected" : "accepted",
        SSL_session_reused(c->ssl) && !c->flag.psk ?
            "previous session reused" : "new session negotiated");

    cipher=(SSL_CIPHER *)SSL_get_current_cipher(c->ssl);
    s_log(LOG_INFO, "%s ciphersuite: %s (%d-bit encryption)",
        SSL_get_version(c->ssl), SSL_CIPHER_get_name(cipher),
        SSL_CIPHER_get_bits(cipher, NULL));

#ifndef OPENSSL_NO_COMP
    compression=SSL_get_current_compression(c->ssl);
    expansion=SSL_get_current_expansion(c->ssl);
    s_log(compression||expansion ? LOG_INFO : LOG_DEBUG,
        "Compression: %s, expansion: %s",
        compression ? SSL_COMP_get_name(compression) : "null",
        expansion ? SSL_COMP_get_name(expansion) : "null");
#endif
}