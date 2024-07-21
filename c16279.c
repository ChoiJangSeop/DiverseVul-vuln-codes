void EVP_MD_CTX_destroy(EVP_MD_CTX *ctx)
	{
	EVP_MD_CTX_cleanup(ctx);
	OPENSSL_free(ctx);
	}