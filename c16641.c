static int check_chain_extensions(X509_STORE_CTX *ctx)
{
    int i, must_be_ca, plen = 0;
    X509 *x;
    int proxy_path_length = 0;
    int purpose;
    int allow_proxy_certs;
    int num = sk_X509_num(ctx->chain);

    /*-
     *  must_be_ca can have 1 of 3 values:
     * -1: we accept both CA and non-CA certificates, to allow direct
     *     use of self-signed certificates (which are marked as CA).
     * 0:  we only accept non-CA certificates.  This is currently not
     *     used, but the possibility is present for future extensions.
     * 1:  we only accept CA certificates.  This is currently used for
     *     all certificates in the chain except the leaf certificate.
     */
    must_be_ca = -1;

    /* CRL path validation */
    if (ctx->parent) {
        allow_proxy_certs = 0;
        purpose = X509_PURPOSE_CRL_SIGN;
    } else {
        allow_proxy_certs =
            ! !(ctx->param->flags & X509_V_FLAG_ALLOW_PROXY_CERTS);
        /*
         * A hack to keep people who don't want to modify their software
         * happy
         */
        if (getenv("OPENSSL_ALLOW_PROXY_CERTS"))
            allow_proxy_certs = 1;
        purpose = ctx->param->purpose;
    }

    for (i = 0; i < num; i++) {
        int ret;
        x = sk_X509_value(ctx->chain, i);
        if (!(ctx->param->flags & X509_V_FLAG_IGNORE_CRITICAL)
            && (x->ex_flags & EXFLAG_CRITICAL)) {
            ctx->error = X509_V_ERR_UNHANDLED_CRITICAL_EXTENSION;
            ctx->error_depth = i;
            ctx->current_cert = x;
            if (!ctx->verify_cb(0, ctx))
                return 0;
        }
        if (!allow_proxy_certs && (x->ex_flags & EXFLAG_PROXY)) {
            ctx->error = X509_V_ERR_PROXY_CERTIFICATES_NOT_ALLOWED;
            ctx->error_depth = i;
            ctx->current_cert = x;
            if (!ctx->verify_cb(0, ctx))
                return 0;
        }
        ret = X509_check_ca(x);
        switch (must_be_ca) {
        case -1:
            if ((ctx->param->flags & X509_V_FLAG_X509_STRICT)
                && (ret != 1) && (ret != 0)) {
                ret = 0;
                ctx->error = X509_V_ERR_INVALID_CA;
            } else
                ret = 1;
            break;
        case 0:
            if (ret != 0) {
                ret = 0;
                ctx->error = X509_V_ERR_INVALID_NON_CA;
            } else
                ret = 1;
            break;
        default:
            if ((ret == 0)
                || ((ctx->param->flags & X509_V_FLAG_X509_STRICT)
                    && (ret != 1))) {
                ret = 0;
                ctx->error = X509_V_ERR_INVALID_CA;
            } else
                ret = 1;
            break;
        }
        if (ret == 0) {
            ctx->error_depth = i;
            ctx->current_cert = x;
            if (! ctx->verify_cb(0, ctx))
                return 0;
        }
        if (purpose > 0) {
            if (!check_purpose(ctx, x, purpose, i, must_be_ca))
                return 0;
        }
        /* Check pathlen if not self issued */
        if ((i > 1) && !(x->ex_flags & EXFLAG_SI)
            && (x->ex_pathlen != -1)
            && (plen > (x->ex_pathlen + proxy_path_length + 1))) {
            ctx->error = X509_V_ERR_PATH_LENGTH_EXCEEDED;
            ctx->error_depth = i;
            ctx->current_cert = x;
            if (!ctx->verify_cb(0, ctx))
                return 0;
        }
        /* Increment path length if not self issued */
        if (!(x->ex_flags & EXFLAG_SI))
            plen++;
        /*
         * If this certificate is a proxy certificate, the next certificate
         * must be another proxy certificate or a EE certificate.  If not,
         * the next certificate must be a CA certificate.
         */
        if (x->ex_flags & EXFLAG_PROXY) {
            if (x->ex_pcpathlen != -1 && i > x->ex_pcpathlen) {
                ctx->error = X509_V_ERR_PROXY_PATH_LENGTH_EXCEEDED;
                ctx->error_depth = i;
                ctx->current_cert = x;
                if (!ctx->verify_cb(0, ctx))
                    return 0;
            }
            proxy_path_length++;
            must_be_ca = 0;
        } else
            must_be_ca = 1;
    }
    return 1;
}