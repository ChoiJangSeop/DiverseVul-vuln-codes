static int drbg_kcapi_random(struct crypto_rng *tfm, u8 *rdata,
			     unsigned int dlen)
{
	struct drbg_state *drbg = crypto_rng_ctx(tfm);
	if (0 < dlen) {
		return drbg_generate_long(drbg, rdata, dlen, NULL);
	} else {
		struct drbg_gen *data = (struct drbg_gen *)rdata;
		struct drbg_string addtl;
		/* catch NULL pointer */
		if (!data)
			return 0;
		drbg_set_testdata(drbg, data->test_data);
		/* linked list variable is now local to allow modification */
		drbg_string_fill(&addtl, data->addtl->buf, data->addtl->len);
		return drbg_generate_long(drbg, data->outbuf, data->outlen,
					  &addtl);
	}
}