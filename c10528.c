rsa_sec_decrypt(const struct rsa_public_key *pub,
	        const struct rsa_private_key *key,
	        void *random_ctx, nettle_random_func *random,
	        size_t length, uint8_t *message,
	        const mpz_t gibberish)
{
  TMP_GMP_DECL (m, mp_limb_t);
  TMP_GMP_DECL (em, uint8_t);
  int res;

  TMP_GMP_ALLOC (m, mpz_size(pub->n));
  TMP_GMP_ALLOC (em, key->size);

  res = _rsa_sec_compute_root_tr (pub, key, random_ctx, random, m,
				  mpz_limbs_read(gibberish),
				  mpz_size(gibberish));

  mpn_get_base256 (em, key->size, m, mpz_size(pub->n));

  res &= _pkcs1_sec_decrypt (length, message, key->size, em);

  TMP_GMP_FREE (em);
  TMP_GMP_FREE (m);
  return res;
}