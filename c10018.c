NOEXPORT int ssl_tlsext_ticket_key_cb(SSL *ssl, unsigned char *key_name,
        unsigned char *iv, EVP_CIPHER_CTX *ctx, HMAC_CTX *hctx, int enc) {
    CLI *c;
    const EVP_CIPHER *cipher;
    int iv_len;

    (void)key_name; /* squash the unused parameter warning */
    s_log(LOG_DEBUG, "Session ticket processing callback");

    c=SSL_get_ex_data(ssl, index_ssl_cli);
    if(!HMAC_Init_ex(hctx, (const unsigned char *)(c->opt->ticket_mac->key_val),
        c->opt->ticket_mac->key_len, EVP_sha256(), NULL)) {
        s_log(LOG_ERR, "HMAC_Init_ex failed");
        return -1;
    }
    if(c->opt->ticket_key->key_len == 16)
        cipher = EVP_aes_128_cbc();
    else /* c->opt->ticket_key->key_len == 32 */
        cipher = EVP_aes_256_cbc();
    if(enc) { /* create new session */
        /* EVP_CIPHER_iv_length() returns 16 for either cipher EVP_aes_128_cbc() or EVP_aes_256_cbc() */
        iv_len = EVP_CIPHER_iv_length(cipher);
        if(RAND_bytes(iv, iv_len) <= 0) { /* RAND_bytes error */
            s_log(LOG_ERR, "RAND_bytes failed");
            return -1;
        }
        if(!EVP_EncryptInit_ex(ctx, cipher, NULL,
            (const unsigned char *)(c->opt->ticket_key->key_val), iv)) {
            s_log(LOG_ERR, "EVP_EncryptInit_ex failed");
            return -1;
        }
    } else /* retrieve session */
        if(!EVP_DecryptInit_ex(ctx, cipher, NULL,
            (const unsigned char *)(c->opt->ticket_key->key_val), iv)) {
            s_log(LOG_ERR, "EVP_DecryptInit_ex failed");
            return -1;
        }
    /* By default, in TLSv1.2 and below, a new session ticket */
    /* is not issued on a successful resumption. */
    /* In TLSv1.3 the default behaviour is to always issue a new ticket on resumption. */
    /* This behaviour can NOT be changed if this ticket key callback is in use! */
    if(strcmp(SSL_get_version(c->ssl), "TLSv1.3"))
        return 1; /* new session ticket is not issued */
    else
        return 2; /* session ticket should be replaced */
}