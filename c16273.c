int FIPS_drbg_init(DRBG_CTX *dctx, int type, unsigned int flags)
	{
	int rv;
	memset(dctx, 0, sizeof(DRBG_CTX));
	dctx->status = DRBG_STATUS_UNINITIALISED;
	dctx->xflags = flags;
	dctx->type = type;

	dctx->iflags = 0;
	dctx->entropy_blocklen = 0;
	dctx->health_check_cnt = 0;
	dctx->health_check_interval = DRBG_HEALTH_INTERVAL;

	rv = fips_drbg_hash_init(dctx);

	if (rv == -2)
		rv = fips_drbg_ctr_init(dctx);
	if (rv == -2)
		rv = fips_drbg_hmac_init(dctx);
	if (rv == -2)
		rv = fips_drbg_ec_init(dctx);

	if (rv <= 0)
		{
		if (rv == -2)
			FIPSerr(FIPS_F_FIPS_DRBG_INIT, FIPS_R_UNSUPPORTED_DRBG_TYPE);
		else
			FIPSerr(FIPS_F_FIPS_DRBG_INIT, FIPS_R_ERROR_INITIALISING_DRBG);
		}

	/* If not in test mode run selftests on DRBG of the same type */

	if (!(dctx->xflags & DRBG_FLAG_TEST))
		{
		if (!FIPS_drbg_health_check(dctx))
			{
			FIPSerr(FIPS_F_FIPS_DRBG_INIT, FIPS_R_SELFTEST_FAILURE);
			return 0;
			}
		}

	return rv;
	}