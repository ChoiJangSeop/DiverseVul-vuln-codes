struct tls_params *tls_initialise(TALLOC_CTX *mem_ctx, struct loadparm_context *lp_ctx)
{
	return talloc_new(mem_ctx);
}