rsa_compute_root_tr(const struct rsa_public_key *pub,
		    const struct rsa_private_key *key,
		    void *random_ctx, nettle_random_func *random,
		    mpz_t x, const mpz_t m)
{
  TMP_GMP_DECL (l, mp_limb_t);
  int res;

  mp_size_t l_size = NETTLE_OCTET_SIZE_TO_LIMB_SIZE(key->size);
  TMP_GMP_ALLOC (l, l_size);

  res = _rsa_sec_compute_root_tr (pub, key, random_ctx, random, l,
				  mpz_limbs_read(m), mpz_size(m));
  if (res) {
    mp_limb_t *xp = mpz_limbs_write (x, l_size);
    mpn_copyi (xp, l, l_size);
    mpz_limbs_finish (x, l_size);
  }

  TMP_GMP_FREE (l);
  return res;
}