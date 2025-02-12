static int get_client_master_key(SSL *s)
{
    int is_export, i, n, keya, ek;
    unsigned long len;
    unsigned char *p;
    const SSL_CIPHER *cp;
    const EVP_CIPHER *c;
    const EVP_MD *md;

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
    i = ssl_rsa_private_decrypt(s->cert, s->s2->tmp.enc,
                                &(p[s->s2->tmp.clear]),
                                &(p[s->s2->tmp.clear]),
                                (s->s2->ssl2_rollback) ? RSA_SSLV23_PADDING :
                                RSA_PKCS1_PADDING);

    is_export = SSL_C_IS_EXPORT(s->session->cipher);

    if (!ssl_cipher_get_evp(s->session, &c, &md, NULL, NULL, NULL)) {
        ssl2_return_error(s, SSL2_PE_NO_CIPHER);
        SSLerr(SSL_F_GET_CLIENT_MASTER_KEY,
               SSL_R_PROBLEMS_MAPPING_CIPHER_FUNCTIONS);
        return (0);
    }

    if (s->session->cipher->algorithm2 & SSL2_CF_8_BYTE_ENC) {
        is_export = 1;
        ek = 8;
    } else
        ek = 5;

    /* bad decrypt */
# if 1
    /*
     * If a bad decrypt, continue with protocol but with a random master
     * secret (Bleichenbacher attack)
     */
    if ((i < 0) || ((!is_export && (i != EVP_CIPHER_key_length(c)))
                    || (is_export && ((i != ek)
                                      || (s->s2->tmp.clear +
                                          (unsigned int)i != (unsigned int)
                                          EVP_CIPHER_key_length(c)))))) {
        ERR_clear_error();
        if (is_export)
            i = ek;
        else
            i = EVP_CIPHER_key_length(c);
        if (RAND_pseudo_bytes(p, i) <= 0)
            return 0;
    }
# else
    if (i < 0) {
        error = 1;
        SSLerr(SSL_F_GET_CLIENT_MASTER_KEY, SSL_R_BAD_RSA_DECRYPT);
    }
    /* incorrect number of key bytes for non export cipher */
    else if ((!is_export && (i != EVP_CIPHER_key_length(c)))
             || (is_export && ((i != ek) || (s->s2->tmp.clear + i !=
                                             EVP_CIPHER_key_length(c))))) {
        error = 1;
        SSLerr(SSL_F_GET_CLIENT_MASTER_KEY, SSL_R_WRONG_NUMBER_OF_KEY_BITS);
    }
    if (error) {
        ssl2_return_error(s, SSL2_PE_UNDEFINED_ERROR);
        return (-1);
    }
# endif

    if (is_export)
        i += s->s2->tmp.clear;

    if (i > SSL_MAX_MASTER_KEY_LENGTH) {
        ssl2_return_error(s, SSL2_PE_UNDEFINED_ERROR);
        SSLerr(SSL_F_GET_CLIENT_MASTER_KEY, ERR_R_INTERNAL_ERROR);
        return -1;
    }
    s->session->master_key_length = i;
    memcpy(s->session->master_key, p, (unsigned int)i);
    return (1);
}