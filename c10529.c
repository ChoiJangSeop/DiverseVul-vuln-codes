_rsa_sec_compute_root_tr(const struct rsa_public_key *pub,
			 const struct rsa_private_key *key,
			 void *random_ctx, nettle_random_func *random,
			 mp_limb_t *x, const mp_limb_t *m, size_t mn)
{
  TMP_GMP_DECL (c, mp_limb_t);
  TMP_GMP_DECL (ri, mp_limb_t);
  TMP_GMP_DECL (scratch, mp_limb_t);
  size_t key_limb_size;
  int ret;

  key_limb_size = NETTLE_OCTET_SIZE_TO_LIMB_SIZE(key->size);

  /* mpz_powm_sec handles only odd moduli. If p, q or n is even, the
     key is invalid and rejected by rsa_private_key_prepare. However,
     some applications, notably gnutls, don't use this function, and
     we don't want an invalid key to lead to a crash down inside
     mpz_powm_sec. So do an additional check here. */
  if (mpz_even_p (pub->n) || mpz_even_p (key->p) || mpz_even_p (key->q))
    {
      mpn_zero(x, key_limb_size);
      return 0;
    }

  assert(mpz_size(pub->n) == key_limb_size);
  assert(mn <= key_limb_size);

  TMP_GMP_ALLOC (c, key_limb_size);
  TMP_GMP_ALLOC (ri, key_limb_size);
  TMP_GMP_ALLOC (scratch, _rsa_sec_compute_root_itch(key));

  rsa_sec_blind (pub, random_ctx, random, x, ri, m, mn);

  _rsa_sec_compute_root(key, c, x, scratch);

  ret = rsa_sec_check_root(pub, c, x);

  rsa_sec_unblind(pub, x, ri, c);

  cnd_mpn_zero(1 - ret, x, key_limb_size);

  TMP_GMP_FREE (scratch);
  TMP_GMP_FREE (ri);
  TMP_GMP_FREE (c);
  return ret;
}