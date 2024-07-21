static int hash_df(DRBG_CTX *dctx, unsigned char *out,
			const unsigned char *in1, size_t in1len,
			const unsigned char *in2, size_t in2len,
			const unsigned char *in3, size_t in3len)
	{
	DRBG_EC_CTX *ectx = &dctx->d.ec;
	EVP_MD_CTX *mctx = &ectx->mctx;
	unsigned char *vtmp = ectx->vtmp;
	unsigned char tmp[6];
	size_t mdlen = M_EVP_MD_size(ectx->md);
	/* Standard only ever needs seedlen bytes which is always less than
	 * maximum permitted so no need to check length.
	 */
	size_t outlen = dctx->seedlen;
	size_t nbits = (outlen << 3) - ectx->exbits;
	tmp[0] = 1;
	tmp[1] = (nbits >> 24) & 0xff;
	tmp[2] = (nbits >> 16) & 0xff;
	tmp[3] = (nbits >> 8) & 0xff;
	tmp[4] = nbits & 0xff;
	if (!in1)
		{
		tmp[5] = (unsigned char)in1len;
		in1 = tmp + 5;
		in1len = 1;
		}
	for (;;)
		{
		if (!FIPS_digestinit(mctx, ectx->md))
			return 0;
		if (!FIPS_digestupdate(mctx, tmp, 5))
			return 0;
		if (in1 && !FIPS_digestupdate(mctx, in1, in1len))
			return 0;
		if (in2 && !FIPS_digestupdate(mctx, in2, in2len))
			return 0;
		if (in3 && !FIPS_digestupdate(mctx, in3, in3len))
			return 0;
		if (outlen < mdlen)
			{
			if (!FIPS_digestfinal(mctx, vtmp, NULL))
				return 0;
			memcpy(out, vtmp, outlen);
			OPENSSL_cleanse(vtmp, mdlen);
			return 1;
			}
		else if(!FIPS_digestfinal(mctx, out, NULL))
			return 0;

		outlen -= mdlen;
		if (outlen == 0)
			return 1;
		tmp[0]++;
		out += mdlen;
		}
	}