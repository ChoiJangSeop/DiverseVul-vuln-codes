rsa_sec_blind (const struct rsa_public_key *pub,
               void *random_ctx, nettle_random_func *random,
               mp_limb_t *c, mp_limb_t *ri, const mp_limb_t *m,
               mp_size_t mn)
{
  const mp_limb_t *ep = mpz_limbs_read (pub->e);
  const mp_limb_t *np = mpz_limbs_read (pub->n);
  mp_bitcnt_t ebn = mpz_sizeinbase (pub->e, 2);
  mp_size_t nn = mpz_size (pub->n);
  size_t itch;
  size_t i2;
  mp_limb_t *scratch;
  TMP_GMP_DECL (tp, mp_limb_t);
  TMP_GMP_DECL (rp, mp_limb_t);
  TMP_GMP_DECL (r, uint8_t);

  TMP_GMP_ALLOC (rp, nn);
  TMP_GMP_ALLOC (r, nn * sizeof(mp_limb_t));

  /* c = m*(r^e) mod n */
  itch = mpn_sec_powm_itch(nn, ebn, nn);
  i2 = mpn_sec_mul_itch(nn, mn);
  itch = MAX(itch, i2);
  i2 = mpn_sec_div_r_itch(nn + mn, nn);
  itch = MAX(itch, i2);
  i2 = mpn_sec_invert_itch(nn);
  itch = MAX(itch, i2);

  TMP_GMP_ALLOC (tp, nn + mn + itch);
  scratch = tp + nn + mn;

  /* ri = r^(-1) */
  do
    {
      random(random_ctx, nn * sizeof(mp_limb_t), (uint8_t *)r);
      mpn_set_base256(rp, nn, r, nn * sizeof(mp_limb_t));
      mpn_copyi(tp, rp, nn);
      /* invert r */
    }
  while (!mpn_sec_invert (ri, tp, np, nn, 2 * nn * GMP_NUMB_BITS, scratch));

  mpn_sec_powm (c, rp, nn, ep, ebn, np, nn, scratch);
  /* normally mn == nn, but m can be smaller in some cases */
  mpn_sec_mul (tp, c, nn, m, mn, scratch);
  mpn_sec_div_r (tp, nn + mn, np, nn, scratch);
  mpn_copyi(c, tp, nn);

  TMP_GMP_FREE (r);
  TMP_GMP_FREE (rp);
  TMP_GMP_FREE (tp);
}