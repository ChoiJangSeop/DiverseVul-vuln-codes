stringprep_utf8_to_ucs4 (const char *str, ssize_t len, size_t * items_written)
{
  return g_utf8_to_ucs4_fast (str, (glong) len, (glong *) items_written);
}