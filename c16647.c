static int get_client_master_key(SSL *s)
{
    int is_export, i, n, keya;
    unsigned int num_encrypted_key_bytes, key_length;
    unsigned long len;
    unsigned char *p;
    const SSL_CIPHER *cp;
    const EVP_CIPHER *c;
    const EVP_MD *md;
    unsigned char rand_premaster_secret[SSL_MAX_MASTER_KEY_LENGTH];
    unsigned char decrypt_good;
    size_t j;

    p = (unsigned char *)s->init_buf->data;
    if (s->state == SSL2_ST_GET_CLIENT_MASTER_KEY_A) {
        i = ssl2_read(s, (char *)&(p[s->init_num]), 10 - s->init_num);

        if (i < (10 - s->init_num))
            return (ssl2_part_read(s, SSL_F_GET_CLIENT_MASTER_KEY, i));
        s->init_num = 10;

        if (*(p++) != SSL2_MT_CLIENT_MASTER_KEY) {
            if (p[-1] != SSL2_MT_ERROR) {
                ssl2_return_error(s, SSL2_PE_UNDEFINED_ERROR);
                SSLerr(SSL_F_GET_CLIENT_MASTER_KEY,
                       SSL_R_READ_WRONG_PACKET_TYPE);
            } else
                SSLerr(SSL_F_GET_CLIENT_MASTER_KEY, SSL_R_PEER_ERROR);
            return (-1);
        }

        cp = ssl2_get_cipher_by_char(p);
        if (cp == NULL) {
            ssl2_return_error(s, SSL2_PE_NO_CIPHER);
            SSLerr(SSL_F_GET_CLIENT_MASTER_KEY, SSL_R_NO_CIPHER_MATCH);
            return (-1);
        }
        s->session->cipher = cp;

        p += 3;
        n2s(p, i);
        s->s2->tmp.clear = i;
        n2s(p, i);
        s->s2->tmp.enc = i;
        n2s(p, i);
        if (i > SSL_MAX_KEY_ARG_LENGTH) {
            ssl2_return_error(s, SSL2_PE_UNDEFINED_ERROR);
            SSLerr(SSL_F_GET_CLIENT_MASTER_KEY, SSL_R_KEY_ARG_TOO_LONG);
            return -1;
        }
        s->session->key_arg_length = i;
        s->state = SSL2_ST_GET_CLIENT_MASTER_KEY_B;
    }

    /* SSL2_ST_GET_CLIENT_MASTER_KEY_B */
    p = (unsigned char *)s->init_buf->data;
    if (s->init_buf->length < SSL2_MAX_RECORD_LENGTH_3_BYTE_HEADER) {
        ssl2_return_error(s, SSL2_PE_UNDEFINED_ERROR);
        SSLerr(SSL_F_GET_CLIENT_MASTER_KEY, ERR_R_INTERNAL_ERROR);
        return -1;
    }
    keya = s->session->key_arg_length;
    len =
        10 + (unsigned long)s->s2->tmp.clear + (unsigned long)s->s2->tmp.enc +
        (unsigned long)keya;
    if (len > SSL2_MAX_RECORD_LENGTH_3_BYTE_HEADER) {
        ssl2_return_error(s, SSL2_PE_UNDEFINED_ERROR);
        SSLerr(SSL_F_GET_CLIENT_MASTER_KEY, SSL_R_MESSAGE_TOO_LONG);
        return -1;
    }
    n = (int)len - s->init_num;
    i = ssl2_read(s, (char *)&(p[s->init_num]), n);
    if (i != n)
        return (ssl2_part_read(s, SSL_F_GET_CLIENT_MASTER_KEY, i));
    if (s->msg_callback) {
        /* CLIENT-MASTER-KEY */
        s->msg_callback(0, s->version, 0, p, (size_t)len, s,
                        s->msg_callback_arg);
    }
    p += 10;

    memcpy(s->session->key_arg, &(p[s->s2->tmp.clear + s->s2->tmp.enc]),
           (unsigned int)keya);

    if (s->cert->pkeys[SSL_PKEY_RSA_ENC].privatekey == NULL) {
        ssl2_return_error(s, SSL2_PE_UNDEFINED_ERROR);
        SSLerr(SSL_F_GET_CLIENT_MASTER_KEY, SSL_R_NO_PRIVATEKEY);
        return (-1);
    }

    is_export = SSL_C_IS_EXPORT(s->session->cipher);

    if (!ssl_cipher_get_evp(s->session, &c, &md, NULL, NULL, NULL)) {
        ssl2_return_error(s, SSL2_PE_NO_CIPHER);
        SSLerr(SSL_F_GET_CLIENT_MASTER_KEY,
               SSL_R_PROBLEMS_MAPPING_CIPHER_FUNCTIONS);
        return (0);
    }

    /*
     * The format of the CLIENT-MASTER-KEY message is
     * 1 byte message type
     * 3 bytes cipher
     * 2-byte clear key length (stored in s->s2->tmp.clear)
     * 2-byte encrypted key length (stored in s->s2->tmp.enc)
     * 2-byte key args length (IV etc)
     * clear key
     * encrypted key
     * key args
     *
     * If the cipher is an export cipher, then the encrypted key bytes
     * are a fixed portion of the total key (5 or 8 bytes). The size of
     * this portion is in |num_encrypted_key_bytes|. If the cipher is not an
     * export cipher, then the entire key material is encrypted (i.e., clear
     * key length must be zero).
     */
    key_length = (unsigned int)EVP_CIPHER_key_length(c);
    if (key_length > SSL_MAX_MASTER_KEY_LENGTH) {
        ssl2_return_error(s, SSL2_PE_UNDEFINED_ERROR);
        SSLerr(SSL_F_GET_CLIENT_MASTER_KEY, ERR_R_INTERNAL_ERROR);
        return -1;
    }

    if (s->session->cipher->algorithm2 & SSL2_CF_8_BYTE_ENC) {
        is_export = 1;
        num_encrypted_key_bytes = 8;
    } else if (is_export) {
        num_encrypted_key_bytes = 5;
    } else {
        num_encrypted_key_bytes = key_length;
    }

    if (s->s2->tmp.clear + num_encrypted_key_bytes != key_length) {
        ssl2_return_error(s, SSL2_PE_UNDEFINED_ERROR);
        SSLerr(SSL_F_GET_CLIENT_MASTER_KEY,SSL_R_BAD_LENGTH);
        return -1;
    }
    /*
     * The encrypted blob must decrypt to the encrypted portion of the key.
     * Decryption can't be expanding, so if we don't have enough encrypted
     * bytes to fit the key in the buffer, stop now.
     */
    if (s->s2->tmp.enc < num_encrypted_key_bytes) {
        ssl2_return_error(s,SSL2_PE_UNDEFINED_ERROR);
        SSLerr(SSL_F_GET_CLIENT_MASTER_KEY,SSL_R_LENGTH_TOO_SHORT);
        return -1;
    }

    /*
     * We must not leak whether a decryption failure occurs because of
     * Bleichenbacher's attack on PKCS #1 v1.5 RSA padding (see RFC 2246,
     * section 7.4.7.1). The code follows that advice of the TLS RFC and
     * generates a random premaster secret for the case that the decrypt
     * fails. See https://tools.ietf.org/html/rfc5246#section-7.4.7.1
     */

    /*
     * should be RAND_bytes, but we cannot work around a failure.
     */
    if (RAND_pseudo_bytes(rand_premaster_secret,
                          (int)num_encrypted_key_bytes) <= 0)
        return 0;

    i = ssl_rsa_private_decrypt(s->cert, s->s2->tmp.enc,
                                &(p[s->s2->tmp.clear]),
                                &(p[s->s2->tmp.clear]),
                                (s->s2->ssl2_rollback) ? RSA_SSLV23_PADDING :
                                RSA_PKCS1_PADDING);
    ERR_clear_error();
    /*
     * If a bad decrypt, continue with protocol but with a random master
     * secret (Bleichenbacher attack)
     */
    decrypt_good = constant_time_eq_int_8(i, (int)num_encrypted_key_bytes);
    for (j = 0; j < num_encrypted_key_bytes; j++) {
        p[s->s2->tmp.clear + j] =
                constant_time_select_8(decrypt_good, p[s->s2->tmp.clear + j],
                                       rand_premaster_secret[j]);
    }

    s->session->master_key_length = (int)key_length;
    memcpy(s->session->master_key, p, key_length);
    OPENSSL_cleanse(p, key_length);

    return 1;
}