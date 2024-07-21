int ssh_dh_generate_y(ssh_session session) {
    session->next_crypto->y = bignum_new();
  if (session->next_crypto->y == NULL) {
    return -1;
  }

#ifdef HAVE_LIBGCRYPT
  bignum_rand(session->next_crypto->y, 128);
#elif defined HAVE_LIBCRYPTO
  bignum_rand(session->next_crypto->y, 128, 0, -1);
#endif

  /* not harder than this */
#ifdef DEBUG_CRYPTO
  ssh_print_bignum("y", session->next_crypto->y);
#endif

  return 0;
}