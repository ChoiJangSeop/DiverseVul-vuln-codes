int ssh_dh_generate_x(ssh_session session) {
  session->next_crypto->x = bignum_new();
  if (session->next_crypto->x == NULL) {
    return -1;
  }

#ifdef HAVE_LIBGCRYPT
  bignum_rand(session->next_crypto->x, 128);
#elif defined HAVE_LIBCRYPTO
  bignum_rand(session->next_crypto->x, 128, 0, -1);
#endif

  /* not harder than this */
#ifdef DEBUG_CRYPTO
  ssh_print_bignum("x", session->next_crypto->x);
#endif

  return 0;
}