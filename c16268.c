static int drbg_ec_uninstantiate(DRBG_CTX *dctx)
	{
	DRBG_EC_CTX *ectx = &dctx->d.ec;
	EVP_MD_CTX_cleanup(&ectx->mctx);
	EC_GROUP_free(ectx->curve);
	EC_POINT_free(ectx->Q);
	EC_POINT_free(ectx->ptmp);
	BN_clear_free(ectx->s);
	BN_CTX_free(ectx->bctx);
	OPENSSL_cleanse(&dctx->d.ec, sizeof(DRBG_EC_CTX));
	return 1;
	}