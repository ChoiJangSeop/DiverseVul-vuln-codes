static gboolean compare_by_guid(gconstpointer a, gconstpointer b) {
  const endpoint_guid * guid_a = (const endpoint_guid *) a;
  const endpoint_guid * guid_b = (const endpoint_guid *) b;
  return memcmp(guid_a, guid_b, sizeof(endpoint_guid)) == 0;
}