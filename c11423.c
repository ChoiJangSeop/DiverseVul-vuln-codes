bool ecdsa_verify_legacy(const ecdsa_verify_context_t *ctx, const ecc_25519_work_t *pubkey) {
  ecc_25519_work_t s2, work;
  ecc_int256_t w, tmp;

  ecc_25519_scalarmult(&s2, &ctx->u2, pubkey);
  ecc_25519_add(&work, &ctx->s1, &s2);
  ecc_25519_store_xy_legacy(&w, NULL, &work);
  ecc_25519_gf_sub(&tmp, &ctx->r, &w);

  return ecc_25519_gf_is_zero(&tmp);
}