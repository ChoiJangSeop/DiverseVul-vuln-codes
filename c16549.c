int tls1_set_server_sigalgs(SSL *s)
{
    int al;
    size_t i;
    /* Clear any shared sigtnature algorithms */
    if (s->cert->shared_sigalgs) {
        OPENSSL_free(s->cert->shared_sigalgs);
        s->cert->shared_sigalgs = NULL;
    }
    /* Clear certificate digests and validity flags */
    for (i = 0; i < SSL_PKEY_NUM; i++) {
        s->cert->pkeys[i].digest = NULL;
        s->cert->pkeys[i].valid_flags = 0;
    }

    /* If sigalgs received process it. */
    if (s->cert->peer_sigalgs) {
        if (!tls1_process_sigalgs(s)) {
            SSLerr(SSL_F_TLS1_SET_SERVER_SIGALGS, ERR_R_MALLOC_FAILURE);
            al = SSL_AD_INTERNAL_ERROR;
            goto err;
        }
        /* Fatal error is no shared signature algorithms */
        if (!s->cert->shared_sigalgs) {
            SSLerr(SSL_F_TLS1_SET_SERVER_SIGALGS,
                   SSL_R_NO_SHARED_SIGATURE_ALGORITHMS);
            al = SSL_AD_ILLEGAL_PARAMETER;
            goto err;
        }
    } else
        ssl_cert_set_default_md(s->cert);
    return 1;
 err:
    ssl3_send_alert(s, SSL3_AL_FATAL, al);
    return 0;
}