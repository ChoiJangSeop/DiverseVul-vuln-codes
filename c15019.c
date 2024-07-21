rsa_oaep_sha256_decrypt(const struct rsa_private_key *key,
	    size_t label_length, const uint8_t *label,
	    size_t *length, uint8_t *message,
	    const mpz_t gibberish)
{
  mpz_t m;
  int res;
  struct sha256_ctx ctx;

  mpz_init(m);
  rsa_compute_root(key, m, gibberish);

  res = pkcs1_oaep_decrypt (key->size, m, SHA256_DIGEST_SIZE,
                            &ctx, &nettle_sha256, (nettle_hash_init_func*)&sha256_init, (nettle_hash_update_func*)&sha256_update, (nettle_hash_digest_func*)&sha256_digest,
                            label_length, label, length, message);
  mpz_clear(m);
  return res;
}