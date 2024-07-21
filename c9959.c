static gboolean compare_by_coherent_set_key(gconstpointer a, gconstpointer b) {
  const coherent_set_key * guid_a = (const coherent_set_key *)a;
  const coherent_set_key * guid_b = (const coherent_set_key *)b;
  return memcmp(guid_a, guid_b, sizeof(coherent_set_key)) == 0;
}