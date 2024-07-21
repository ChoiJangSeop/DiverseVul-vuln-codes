static int check_purpose(X509_STORE_CTX *ctx, X509 *x, int purpose, int depth,
                         int must_be_ca)
{
    int pu_ok = X509_check_purpose(x, purpose, must_be_ca > 0);
    int tr_ok = X509_TRUST_UNTRUSTED;

    /*
     * For trusted certificates we want to see whether any auxiliary trust
     * settings override the purpose constraints we failed to meet above.
     *
     * This is complicated by the fact that the trust ordinals in
     * ctx->param->trust are entirely independent of the purpose ordinals in
     * ctx->param->purpose!
     *
     * What connects them is their mutual initialization via calls from
     * X509_STORE_CTX_set_default() into X509_VERIFY_PARAM_lookup() which sets
     * related values of both param->trust and param->purpose.  It is however
     * typically possible to infer associated trust values from a purpose value
     * via the X509_PURPOSE API.
     *
     * Therefore, we can only check for trust overrides when the purpose we're
     * checking is the same as ctx->param->purpose and ctx->param->trust is
     * also set, or can be inferred from the purpose.
     */
    if (depth >= ctx->num_untrusted && purpose == ctx->param->purpose)
        tr_ok = X509_check_trust(x, ctx->param->trust, X509_TRUST_NO_SS_COMPAT);

    if (tr_ok != X509_TRUST_REJECTED &&
        (pu_ok == 1 ||
         (pu_ok != 0 && (ctx->param->flags & X509_V_FLAG_X509_STRICT) == 0)))
        return 1;

    ctx->error = X509_V_ERR_INVALID_PURPOSE;
    ctx->error_depth = depth;
    ctx->current_cert = x;
    return ctx->verify_cb(0, ctx);
}