WORK_STATE tls_post_process_server_certificate(SSL *s, WORK_STATE wst)
{
    X509 *x;
    EVP_PKEY *pkey = NULL;
    const SSL_CERT_LOOKUP *clu;
    size_t certidx;
    int i;

    i = ssl_verify_cert_chain(s, s->session->peer_chain);
    if (i == -1) {
        s->rwstate = SSL_RETRY_VERIFY;
        return WORK_MORE_A;
    }
    /*
     * The documented interface is that SSL_VERIFY_PEER should be set in order
     * for client side verification of the server certificate to take place.
     * However, historically the code has only checked that *any* flag is set
     * to cause server verification to take place. Use of the other flags makes
     * no sense in client mode. An attempt to clean up the semantics was
     * reverted because at least one application *only* set
     * SSL_VERIFY_FAIL_IF_NO_PEER_CERT. Prior to the clean up this still caused
     * server verification to take place, after the clean up it silently did
     * nothing. SSL_CTX_set_verify()/SSL_set_verify() cannot validate the flags
     * sent to them because they are void functions. Therefore, we now use the
     * (less clean) historic behaviour of performing validation if any flag is
     * set. The *documented* interface remains the same.
     */
    if (s->verify_mode != SSL_VERIFY_NONE && i <= 0) {
        SSLfatal(s, ssl_x509err2alert(s->verify_result),
                 SSL_R_CERTIFICATE_VERIFY_FAILED);
        return WORK_ERROR;
    }
    ERR_clear_error();          /* but we keep s->verify_result */

    /*
     * Inconsistency alert: cert_chain does include the peer's certificate,
     * which we don't include in statem_srvr.c
     */
    x = sk_X509_value(s->session->peer_chain, 0);

    pkey = X509_get0_pubkey(x);

    if (pkey == NULL || EVP_PKEY_missing_parameters(pkey)) {
        SSLfatal(s, SSL_AD_INTERNAL_ERROR,
                 SSL_R_UNABLE_TO_FIND_PUBLIC_KEY_PARAMETERS);
        return WORK_ERROR;
    }

    if ((clu = ssl_cert_lookup_by_pkey(pkey, &certidx)) == NULL) {
        SSLfatal(s, SSL_AD_ILLEGAL_PARAMETER, SSL_R_UNKNOWN_CERTIFICATE_TYPE);
        return WORK_ERROR;
    }
    /*
     * Check certificate type is consistent with ciphersuite. For TLS 1.3
     * skip check since TLS 1.3 ciphersuites can be used with any certificate
     * type.
     */
    if (!SSL_IS_TLS13(s)) {
        if ((clu->amask & s->s3.tmp.new_cipher->algorithm_auth) == 0) {
            SSLfatal(s, SSL_AD_ILLEGAL_PARAMETER, SSL_R_WRONG_CERTIFICATE_TYPE);
            return WORK_ERROR;
        }
    }

    X509_free(s->session->peer);
    X509_up_ref(x);
    s->session->peer = x;
    s->session->verify_result = s->verify_result;

    /* Save the current hash state for when we receive the CertificateVerify */
    if (SSL_IS_TLS13(s)
            && !ssl_handshake_hash(s, s->cert_verify_hash,
                                   sizeof(s->cert_verify_hash),
                                   &s->cert_verify_hash_len)) {
        /* SSLfatal() already called */;
        return WORK_ERROR;
    }
    return WORK_FINISHED_CONTINUE;
}