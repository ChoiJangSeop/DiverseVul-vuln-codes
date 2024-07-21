static char *fstrndup(const char *ptr, unsigned long len) {
  char *result;
  if (len <= 0) return NULL;
  result = ALLOC_N(char, len);
  memccpy(result, ptr, 0, len);
  return result;
}