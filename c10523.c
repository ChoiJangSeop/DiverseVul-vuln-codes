_rsa_sec_compute_root_tr(const struct rsa_public_key *pub,
			 const struct rsa_private_key *key,
			 void *random_ctx, nettle_random_func *random,
			 mp_limb_t *x, const mp_limb_t *m, size_t mn)
{
  mpz_t mz;
  mpz_t xz;
  int res;

  mpz_init(mz);
  mpz_init(xz);

  mpn_copyi(mpz_limbs_write(mz, mn), m, mn);
  mpz_limbs_finish(mz, mn);

  res = rsa_compute_root_tr(pub, key, random_ctx, random, xz, mz);

  if (res)
    mpz_limbs_copy(x, xz, mpz_size(pub->n));

  mpz_clear(mz);
  mpz_clear(xz);
  return res;
}