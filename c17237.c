int ssl_verify_cert_chain(SSL *s, STACK_OF(X509) *sk)
{
    X509 *x;
    int i = 0;
    X509_STORE *verify_store;
    X509_STORE_CTX *ctx = NULL;
    X509_VERIFY_PARAM *param;

    if ((sk == NULL) || (sk_X509_num(sk) == 0))
        return 0;

    if (s->cert->verify_store)
        verify_store = s->cert->verify_store;
    else
        verify_store = s->ctx->cert_store;

    ctx = X509_STORE_CTX_new_ex(s->ctx->libctx, s->ctx->propq);
    if (ctx == NULL) {
        ERR_raise(ERR_LIB_SSL, ERR_R_MALLOC_FAILURE);
        return 0;
    }

    x = sk_X509_value(sk, 0);
    if (!X509_STORE_CTX_init(ctx, verify_store, x, sk)) {
        ERR_raise(ERR_LIB_SSL, ERR_R_X509_LIB);
        goto end;
    }
    param = X509_STORE_CTX_get0_param(ctx);
    /*
     * XXX: Separate @AUTHSECLEVEL and @TLSSECLEVEL would be useful at some
     * point, for now a single @SECLEVEL sets the same policy for TLS crypto
     * and PKI authentication.
     */
    X509_VERIFY_PARAM_set_auth_level(param, SSL_get_security_level(s));

    /* Set suite B flags if needed */
    X509_STORE_CTX_set_flags(ctx, tls1_suiteb(s));
    if (!X509_STORE_CTX_set_ex_data
        (ctx, SSL_get_ex_data_X509_STORE_CTX_idx(), s)) {
        goto end;
    }

    /* Verify via DANE if enabled */
    if (DANETLS_ENABLED(&s->dane))
        X509_STORE_CTX_set0_dane(ctx, &s->dane);

    /*
     * We need to inherit the verify parameters. These can be determined by
     * the context: if its a server it will verify SSL client certificates or
     * vice versa.
     */

    X509_STORE_CTX_set_default(ctx, s->server ? "ssl_client" : "ssl_server");
    /*
     * Anything non-default in "s->param" should overwrite anything in the ctx.
     */
    X509_VERIFY_PARAM_set1(param, s->param);

    if (s->verify_callback)
        X509_STORE_CTX_set_verify_cb(ctx, s->verify_callback);

    if (s->ctx->app_verify_callback != NULL)
        i = s->ctx->app_verify_callback(ctx, s->ctx->app_verify_arg);
    else
        i = X509_verify_cert(ctx);

    s->verify_result = X509_STORE_CTX_get_error(ctx);
    sk_X509_pop_free(s->verified_chain, X509_free);
    s->verified_chain = NULL;
    if (X509_STORE_CTX_get0_chain(ctx) != NULL) {
        s->verified_chain = X509_STORE_CTX_get1_chain(ctx);
        if (s->verified_chain == NULL) {
            ERR_raise(ERR_LIB_SSL, ERR_R_MALLOC_FAILURE);
            i = 0;
        }
    }

    /* Move peername from the store context params to the SSL handle's */
    X509_VERIFY_PARAM_move_peername(s->param, param);

 end:
    X509_STORE_CTX_free(ctx);
    return i;
}