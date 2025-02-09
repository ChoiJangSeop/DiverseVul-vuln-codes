struct crypto_alg *crypto_larval_lookup(const char *name, u32 type, u32 mask)
{
	struct crypto_alg *alg;

	if (!name)
		return ERR_PTR(-ENOENT);

	mask &= ~(CRYPTO_ALG_LARVAL | CRYPTO_ALG_DEAD);
	type &= mask;

	alg = crypto_alg_lookup(name, type, mask);
	if (!alg) {
		request_module("%s", name);

		if (!((type ^ CRYPTO_ALG_NEED_FALLBACK) & mask &
		      CRYPTO_ALG_NEED_FALLBACK))
			request_module("%s-all", name);

		alg = crypto_alg_lookup(name, type, mask);
	}

	if (alg)
		return crypto_is_larval(alg) ? crypto_larval_wait(alg) : alg;

	return crypto_larval_add(name, type, mask);
}