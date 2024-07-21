void ecdsa_verify_prepare_legacy(ecdsa_verify_context_t *ctx, const ecc_int256_t *hash, const ecdsa_signature_t *signature) {
  ecc_int256_t w, u1, tmp;

  ctx->r = signature->r;

  ecc_25519_gf_recip(&w, &signature->s);
  ecc_25519_gf_reduce(&tmp, hash);
  ecc_25519_gf_mult(&u1, &tmp, &w);
  ecc_25519_gf_mult(&ctx->u2, &ctx->r, &w);
  ecc_25519_scalarmult_base(&ctx->s1, &u1);
}