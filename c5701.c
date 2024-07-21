static int drbg_seed(struct drbg_state *drbg, struct drbg_string *pers,
		     bool reseed)
{
	int ret = 0;
	unsigned char *entropy = NULL;
	size_t entropylen = 0;
	struct drbg_string data1;
	LIST_HEAD(seedlist);

	/* 9.1 / 9.2 / 9.3.1 step 3 */
	if (pers && pers->len > (drbg_max_addtl(drbg))) {
		pr_devel("DRBG: personalization string too long %zu\n",
			 pers->len);
		return -EINVAL;
	}

	if (drbg->test_data && drbg->test_data->testentropy) {
		drbg_string_fill(&data1, drbg->test_data->testentropy->buf,
				 drbg->test_data->testentropy->len);
		pr_devel("DRBG: using test entropy\n");
	} else {
		/*
		 * Gather entropy equal to the security strength of the DRBG.
		 * With a derivation function, a nonce is required in addition
		 * to the entropy. A nonce must be at least 1/2 of the security
		 * strength of the DRBG in size. Thus, entropy * nonce is 3/2
		 * of the strength. The consideration of a nonce is only
		 * applicable during initial seeding.
		 */
		entropylen = drbg_sec_strength(drbg->core->flags);
		if (!entropylen)
			return -EFAULT;
		if (!reseed)
			entropylen = ((entropylen + 1) / 2) * 3;
		pr_devel("DRBG: (re)seeding with %zu bytes of entropy\n",
			 entropylen);
		entropy = kzalloc(entropylen, GFP_KERNEL);
		if (!entropy)
			return -ENOMEM;
		get_random_bytes(entropy, entropylen);
		drbg_string_fill(&data1, entropy, entropylen);
	}
	list_add_tail(&data1.list, &seedlist);

	/*
	 * concatenation of entropy with personalization str / addtl input)
	 * the variable pers is directly handed in by the caller, so check its
	 * contents whether it is appropriate
	 */
	if (pers && pers->buf && 0 < pers->len) {
		list_add_tail(&pers->list, &seedlist);
		pr_devel("DRBG: using personalization string\n");
	}

	if (!reseed) {
		memset(drbg->V, 0, drbg_statelen(drbg));
		memset(drbg->C, 0, drbg_statelen(drbg));
	}

	ret = drbg->d_ops->update(drbg, &seedlist, reseed);
	if (ret)
		goto out;

	drbg->seeded = true;
	/* 10.1.1.2 / 10.1.1.3 step 5 */
	drbg->reseed_ctr = 1;

out:
	kzfree(entropy);
	return ret;
}