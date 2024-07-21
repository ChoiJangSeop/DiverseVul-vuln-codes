static int drbg_ec_reseed(DRBG_CTX *dctx,
				const unsigned char *ent, size_t ent_len,
				const unsigned char *adin, size_t adin_len)
	{
	DRBG_EC_CTX *ectx = &dctx->d.ec;
	/* Convert s value to a binary buffer. Save it to tbuf as we are
	 * about to overwrite it.
	 */
	if (ectx->exbits)
		BN_lshift(ectx->s, ectx->s, ectx->exbits);
	bn2binpad(ectx->tbuf, dctx->seedlen, ectx->s);
	if (!hash_df(dctx, ectx->sbuf, ectx->tbuf, dctx->seedlen, 
			ent, ent_len, adin, adin_len))
		return 0;
	if (!bin2bnbits(dctx, ectx->s, ectx->sbuf))
		return 0;
	dctx->reseed_counter = 0;
	return 1;
	}