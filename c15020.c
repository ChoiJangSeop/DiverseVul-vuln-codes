rsa_oaep_sha1_decrypt(const struct rsa_private_key *key,
	    size_t label_length, const uint8_t *label,
	    size_t *length, uint8_t *message,
	    const mpz_t gibberish)
{
  mpz_t m;
  int res;
  struct sha1_ctx ctx;

  mpz_init(m);
  rsa_compute_root(key, m, gibberish);

  res = pkcs1_oaep_decrypt (key->size, m, SHA1_DIGEST_SIZE,
                            &ctx, &nettle_sha1, (nettle_hash_init_func*)&sha1_init, (nettle_hash_update_func*)&sha1_update, (nettle_hash_digest_func*)&sha1_digest,
                            label_length, label, length, message);
  mpz_clear(m);
  return res;
}