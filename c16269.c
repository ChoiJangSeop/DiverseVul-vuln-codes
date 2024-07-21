static int bin2bnbits(DRBG_CTX *dctx, BIGNUM *r, const unsigned char *buf)
	{
	DRBG_EC_CTX *ectx = &dctx->d.ec;
	if (!BN_bin2bn(buf, dctx->seedlen, r))
		return 0;
	/* If we have extra bits right shift off the end of r */
	if (ectx->exbits)
		{
		if (!BN_rshift(r, r, ectx->exbits))
			return 0;
		}
	return 1;
	}