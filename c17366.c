static void zlib_stateful_finish(COMP_CTX *ctx)
	{
	CRYPTO_free_ex_data(CRYPTO_EX_INDEX_COMP,ctx,&ctx->ex_data);
	}