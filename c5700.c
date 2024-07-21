static int drbg_kcapi_reset(struct crypto_rng *tfm, u8 *seed, unsigned int slen)
{
	struct drbg_state *drbg = crypto_rng_ctx(tfm);
	struct crypto_tfm *tfm_base = crypto_rng_tfm(tfm);
	bool pr = false;
	struct drbg_string seed_string;
	int coreref = 0;

	drbg_convert_tfm_core(crypto_tfm_alg_driver_name(tfm_base), &coreref,
			      &pr);
	if (0 < slen) {
		drbg_string_fill(&seed_string, seed, slen);
		return drbg_instantiate(drbg, &seed_string, coreref, pr);
	} else {
		struct drbg_gen *data = (struct drbg_gen *)seed;
		/* allow invocation of API call with NULL, 0 */
		if (!data)
			return drbg_instantiate(drbg, NULL, coreref, pr);
		drbg_set_testdata(drbg, data->test_data);
		/* linked list variable is now local to allow modification */
		drbg_string_fill(&seed_string, data->addtl->buf,
				 data->addtl->len);
		return drbg_instantiate(drbg, &seed_string, coreref, pr);
	}
}