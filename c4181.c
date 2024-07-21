dsa_sign(const struct dsa_params *params,
	 const mpz_t x,
	 void *random_ctx, nettle_random_func *random,
	 size_t digest_size,
	 const uint8_t *digest,
	 struct dsa_signature *signature)
{
  mpz_t k;
  mpz_t h;
  mpz_t tmp;
  int res;
  
  /* Select k, 0<k<q, randomly */
  mpz_init_set(tmp, params->q);
  mpz_sub_ui(tmp, tmp, 1);

  mpz_init(k);
  nettle_mpz_random(k, random_ctx, random, tmp);
  mpz_add_ui(k, k, 1);

  /* Compute r = (g^k (mod p)) (mod q) */
  mpz_powm(tmp, params->g, k, params->p);
  mpz_fdiv_r(signature->r, tmp, params->q);

  /* Compute hash */
  mpz_init(h);
  _dsa_hash (h, mpz_sizeinbase(params->q, 2), digest_size, digest);

  /* Compute k^-1 (mod q) */
  if (mpz_invert(k, k, params->q))
    {
      /* Compute signature s = k^-1 (h + xr) (mod q) */
      mpz_mul(tmp, signature->r, x);
      mpz_fdiv_r(tmp, tmp, params->q);
      mpz_add(tmp, tmp, h);
      mpz_mul(tmp, tmp, k);
      mpz_fdiv_r(signature->s, tmp, params->q);
      res = 1;
    }
  else
    /* What do we do now? The key is invalid. */
    res = 0;

  mpz_clear(k);
  mpz_clear(h);
  mpz_clear(tmp);

  return res;
}