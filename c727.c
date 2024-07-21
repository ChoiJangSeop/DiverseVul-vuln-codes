tor_tls_context_init_one(tor_tls_context_t **ppcontext,
                         crypto_pk_env_t *identity,
                         unsigned int key_lifetime)
{
  tor_tls_context_t *new_ctx = tor_tls_context_new(identity,
                                                   key_lifetime);
  tor_tls_context_t *old_ctx = *ppcontext;

  if (new_ctx != NULL) {
    *ppcontext = new_ctx;

    /* Free the old context if one existed. */
    if (old_ctx != NULL) {
      /* This is safe even if there are open connections: we reference-
       * count tor_tls_context_t objects. */
      tor_tls_context_decref(old_ctx);
    }
  }

  return ((new_ctx != NULL) ? 0 : -1);
}