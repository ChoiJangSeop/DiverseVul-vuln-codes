static int drbg_ec_instantiate(DRBG_CTX *dctx,
				const unsigned char *ent, size_t ent_len,
				const unsigned char *nonce, size_t nonce_len,
				const unsigned char *pstr, size_t pstr_len)
	{
	DRBG_EC_CTX *ectx = &dctx->d.ec;
	if (!hash_df(dctx, ectx->sbuf, 
			ent, ent_len, nonce, nonce_len, pstr, pstr_len))
		return 0;
	if (!bin2bnbits(dctx, ectx->s, ectx->sbuf))
		return 0;
	return 1;
	}