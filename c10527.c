rsa_compute_root_tr(const struct rsa_public_key *pub,
		    const struct rsa_private_key *key,
		    void *random_ctx, nettle_random_func *random,
		    mpz_t x, const mpz_t m)
{
  int res;
  mpz_t t, mb, xb, ri;

  /* mpz_powm_sec handles only odd moduli. If p, q or n is even, the
     key is invalid and rejected by rsa_private_key_prepare. However,
     some applications, notably gnutls, don't use this function, and
     we don't want an invalid key to lead to a crash down inside
     mpz_powm_sec. So do an additional check here. */
  if (mpz_even_p (pub->n) || mpz_even_p (key->p) || mpz_even_p (key->q))
    return 0;

  mpz_init (mb);
  mpz_init (xb);
  mpz_init (ri);
  mpz_init (t);

  rsa_blind (pub, random_ctx, random, mb, ri, m);

  rsa_compute_root (key, xb, mb);

  mpz_powm_sec(t, xb, pub->e, pub->n);
  res = (mpz_cmp(mb, t) == 0);

  if (res)
    rsa_unblind (pub, x, ri, xb);

  mpz_clear (mb);
  mpz_clear (xb);
  mpz_clear (ri);
  mpz_clear (t);

  return res;
}