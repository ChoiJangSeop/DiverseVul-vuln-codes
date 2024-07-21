rsa_decrypt(const struct rsa_private_key *key,
	    size_t *length, uint8_t *message,
	    const mpz_t gibberish)
{
  mpz_t m;
  int res;

  mpz_init(m);
  rsa_compute_root(key, m, gibberish);

  res = pkcs1_decrypt (key->size, m, length, message);
  mpz_clear(m);
  return res;
}