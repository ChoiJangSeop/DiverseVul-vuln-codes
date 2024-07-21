static int drbg_ec_generate(DRBG_CTX *dctx,
				unsigned char *out, size_t outlen,
				const unsigned char *adin, size_t adin_len)
	{
	DRBG_EC_CTX *ectx = &dctx->d.ec;
	BIGNUM *t, *r;
	BIGNUM *s = ectx->s;
	/* special case: check reseed interval */
	if (out == NULL)
		{
		size_t nb = (outlen + dctx->blocklength - 1)/dctx->blocklength;
		if (dctx->reseed_counter + nb > dctx->reseed_interval)
			dctx->status = DRBG_STATUS_RESEED;
		return 1;
		}

	BN_CTX_start(ectx->bctx);
	r = BN_CTX_get(ectx->bctx);
	if (!r)
		goto err;
	if (adin && adin_len)
		{
		size_t i;
		t = BN_CTX_get(ectx->bctx);
		if (!t)
			goto err;
		/* Convert s to buffer */
		if (ectx->exbits)
			BN_lshift(s, s, ectx->exbits);
		bn2binpad(ectx->sbuf, dctx->seedlen, s);
		/* Step 2 */
		if (!hash_df(dctx, ectx->tbuf, adin, adin_len,
				NULL, 0, NULL, 0))
			goto err;
		/* Step 5 */
		for (i = 0; i < dctx->seedlen; i++)
			ectx->tbuf[i] ^= ectx->sbuf[i];
		if (!bin2bnbits(dctx, t, ectx->tbuf))
			return 0;
		}
	else
		/* Note if no additional input the algorithm never
		 * needs separate values for t and s.
		 */
		t = s;

#ifdef EC_DRBG_TRACE
	bnprint(stderr, "s at start of generate: ", s);
#endif

	for (;;)
		{
		/* Step #6, calculate s = t * P */
		if (!drbg_ec_mul(ectx, s, t, 0))
			goto err;
#ifdef EC_DRBG_TRACE
		bnprint(stderr, "s in generate: ", ectx->s);
#endif
		/* Step #7, calculate r = s * Q */
		if (!drbg_ec_mul(ectx, r, s, 1))
			goto err;
#ifdef EC_DRBG_TRACE
	bnprint(stderr, "r in generate is: ", r);
#endif
		dctx->reseed_counter++;
		/* Get rightmost bits of r to output buffer */

		if (!(dctx->xflags & DRBG_FLAG_TEST) && !dctx->lb_valid)
			{
			if (!bn2binpad(dctx->lb, dctx->blocklength, r))
				goto err;
			dctx->lb_valid = 1;
			continue;
			}
		if (outlen < dctx->blocklength)
			{
			if (!bn2binpad(ectx->vtmp, dctx->blocklength, r))
				goto err;
			if (!fips_drbg_cprng_test(dctx, ectx->vtmp))
				goto err;
			memcpy(out, ectx->vtmp, outlen);
			break;
			}
		else
			{
			if (!bn2binpad(out, dctx->blocklength, r))
				goto err;
			if (!fips_drbg_cprng_test(dctx, out))
				goto err;
			}	
		outlen -= dctx->blocklength;
		if (!outlen)
			break;
		out += dctx->blocklength;
		/* Step #5 after first pass */
		t = s;
#ifdef EC_DRBG_TRACE
		fprintf(stderr, "Random bits written:\n");
		hexprint(stderr, out, dctx->blocklength);
#endif
		}
	if (!drbg_ec_mul(ectx, ectx->s, ectx->s, 0))
		return 0;
#ifdef EC_DRBG_TRACE
	bnprint(stderr, "s after generate is: ", s);
#endif
	BN_CTX_end(ectx->bctx);
	return 1;
	err:
	BN_CTX_end(ectx->bctx);
	return 0;
	}