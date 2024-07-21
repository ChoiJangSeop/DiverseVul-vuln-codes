_rsa_blind (const struct rsa_public_key *pub,
	    void *random_ctx, nettle_random_func *random,
	    mpz_t c, mpz_t ri)
{
  mpz_t r;

  mpz_init(r);

  /* c = c*(r^e)
   * ri = r^(-1)
   */
  do 
    {
      nettle_mpz_random(r, random_ctx, random, pub->n);
      /* invert r */
    }
  while (!mpz_invert (ri, r, pub->n));

  /* c = c*(r^e) mod n */
  mpz_powm(r, r, pub->e, pub->n);
  mpz_mul(c, c, r);
  mpz_fdiv_r(c, c, pub->n);

  mpz_clear(r);
}