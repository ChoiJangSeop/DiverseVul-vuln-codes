unsigned uv__utf8_decode1(const char** p, const char* pe) {
  unsigned a;

  a = (unsigned char) *(*p)++;

  if (a < 128)
    return a;  /* ASCII, common case. */

  return uv__utf8_decode1_slow(p, pe, a);
}