static guint coherent_set_key_hash_by_key(gconstpointer key) {
  GBytes * coherent_set_object_key_bytes = NULL;
  coherent_set_object_key_bytes = g_bytes_new(key, sizeof(coherent_set_key));
  return g_bytes_hash(coherent_set_object_key_bytes);
}