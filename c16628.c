static int check_trust(X509_STORE_CTX *ctx)
{
#ifdef OPENSSL_NO_CHAIN_VERIFY
	return 1;
#else
	int i, ok;
	X509 *x;
	int (*cb)(int xok,X509_STORE_CTX *xctx);
	cb=ctx->verify_cb;
/* For now just check the last certificate in the chain */
	i = sk_X509_num(ctx->chain) - 1;
	x = sk_X509_value(ctx->chain, i);
	ok = X509_check_trust(x, ctx->param->trust, 0);
	if (ok == X509_TRUST_TRUSTED)
		return 1;
	ctx->error_depth = i;
	ctx->current_cert = x;
	if (ok == X509_TRUST_REJECTED)
		ctx->error = X509_V_ERR_CERT_REJECTED;
	else
		ctx->error = X509_V_ERR_CERT_UNTRUSTED;
	ok = cb(0, ctx);
	return ok;
#endif
}