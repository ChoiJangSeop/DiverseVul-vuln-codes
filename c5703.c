static bool drbg_fips_continuous_test(struct drbg_state *drbg,
				      const unsigned char *buf)
{
#ifdef CONFIG_CRYPTO_FIPS
	int ret = 0;
	/* skip test if we test the overall system */
	if (drbg->test_data)
		return true;
	/* only perform test in FIPS mode */
	if (0 == fips_enabled)
		return true;
	if (!drbg->fips_primed) {
		/* Priming of FIPS test */
		memcpy(drbg->prev, buf, drbg_blocklen(drbg));
		drbg->fips_primed = true;
		/* return false due to priming, i.e. another round is needed */
		return false;
	}
	ret = memcmp(drbg->prev, buf, drbg_blocklen(drbg));
	if (!ret)
		panic("DRBG continuous self test failed\n");
	memcpy(drbg->prev, buf, drbg_blocklen(drbg));
	/* the test shall pass when the two compared values are not equal */
	return ret != 0;
#else
	return true;
#endif /* CONFIG_CRYPTO_FIPS */
}