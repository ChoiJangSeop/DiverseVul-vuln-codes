tor_tls_context_init(int is_public_server,
                     crypto_pk_env_t *client_identity,
                     crypto_pk_env_t *server_identity,
                     unsigned int key_lifetime)
{
  int rv1 = 0;
  int rv2 = 0;

  if (is_public_server) {
    tor_tls_context_t *new_ctx;
    tor_tls_context_t *old_ctx;

    tor_assert(server_identity != NULL);

    rv1 = tor_tls_context_init_one(&server_tls_context,
                                   server_identity,
                                   key_lifetime);

    if (rv1 >= 0) {
      new_ctx = server_tls_context;
      tor_tls_context_incref(new_ctx);
      old_ctx = client_tls_context;
      client_tls_context = new_ctx;

      if (old_ctx != NULL) {
        tor_tls_context_decref(old_ctx);
      }
    }
  } else {
    if (server_identity != NULL) {
      rv1 = tor_tls_context_init_one(&server_tls_context,
                                     server_identity,
                                     key_lifetime);
    } else {
      tor_tls_context_t *old_ctx = server_tls_context;
      server_tls_context = NULL;

      if (old_ctx != NULL) {
        tor_tls_context_decref(old_ctx);
      }
    }

    rv2 = tor_tls_context_init_one(&client_tls_context,
                                   client_identity,
                                   key_lifetime);
  }

  return rv1 < rv2 ? rv1 : rv2;
}