rsa_compute_root(const struct rsa_private_key *key,
		 mpz_t x, const mpz_t m)
{
  mpz_t xp; /* modulo p */
  mpz_t xq; /* modulo q */

  mpz_init(xp); mpz_init(xq);    

  /* Compute xq = m^d % q = (m%q)^b % q */
  mpz_fdiv_r(xq, m, key->q);
  mpz_powm(xq, xq, key->b, key->q);

  /* Compute xp = m^d % p = (m%p)^a % p */
  mpz_fdiv_r(xp, m, key->p);
  mpz_powm(xp, xp, key->a, key->p);

  /* Set xp' = (xp - xq) c % p. */
  mpz_sub(xp, xp, xq);
  mpz_mul(xp, xp, key->c);
  mpz_fdiv_r(xp, xp, key->p);

  /* Finally, compute x = xq + q xp'
   *
   * To prove that this works, note that
   *
   *   xp  = x + i p,
   *   xq  = x + j q,
   *   c q = 1 + k p
   *
   * for some integers i, j and k. Now, for some integer l,
   *
   *   xp' = (xp - xq) c + l p
   *       = (x + i p - (x + j q)) c + l p
   *       = (i p - j q) c + l p
   *       = (i c + l) p - j (c q)
   *       = (i c + l) p - j (1 + kp)
   *       = (i c + l - j k) p - j
   *
   * which shows that xp' = -j (mod p). We get
   *
   *   xq + q xp' = x + j q + (i c + l - j k) p q - j q
   *              = x + (i c + l - j k) p q
   *
   * so that
   *
   *   xq + q xp' = x (mod pq)
   *
   * We also get 0 <= xq + q xp' < p q, because
   *
   *   0 <= xq < q and 0 <= xp' < p.
   */
  mpz_mul(x, key->q, xp);
  mpz_add(x, x, xq);

  mpz_clear(xp); mpz_clear(xq);
}