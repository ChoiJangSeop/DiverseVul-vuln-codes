static int crypto_rng_init_tfm(struct crypto_tfm *tfm)
{
	struct crypto_rng *rng = __crypto_rng_cast(tfm);
	struct rng_alg *alg = crypto_rng_alg(rng);
	struct old_rng_alg *oalg = crypto_old_rng_alg(rng);

	if (oalg->rng_make_random) {
		rng->generate = generate;
		rng->seed = rngapi_reset;
		rng->seedsize = oalg->seedsize;
		return 0;
	}

	rng->generate = alg->generate;
	rng->seed = alg->seed;
	rng->seedsize = alg->seedsize;

	return 0;
}