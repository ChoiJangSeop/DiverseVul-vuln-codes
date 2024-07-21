rsa_decrypt_tr(const struct rsa_public_key *pub,
	       const struct rsa_private_key *key,
	       void *random_ctx, nettle_random_func *random,
	       size_t *length, uint8_t *message,
	       const mpz_t gibberish)
{
  TMP_GMP_DECL (m, mp_limb_t);
  TMP_GMP_DECL (em, uint8_t);
  mp_size_t key_limb_size;
  int res;

  key_limb_size = mpz_size(pub->n);

  TMP_GMP_ALLOC (m, key_limb_size);
  TMP_GMP_ALLOC (em, key->size);
  mpz_limbs_copy(m, gibberish, key_limb_size);

  res = _rsa_sec_compute_root_tr (pub, key, random_ctx, random, m, m);

  mpn_get_base256 (em, key->size, m, key_limb_size);

  res &= _pkcs1_sec_decrypt_variable (length, message, key->size, em);

  TMP_GMP_FREE (em);
  TMP_GMP_FREE (m);
  return res;
}