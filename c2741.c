translate_hex_string(char *s, char *saved_orphan)
{
  int c1 = *saved_orphan;
  char *start = s;
  char *t = s;
  for (; *s; s++) {
    if (isspace(*s))
      continue;
    if (c1) {
      *t++ = (hexval(c1) << 4) + hexval(*s);
      c1 = 0;
    } else
      c1 = *s;
  }
  *saved_orphan = c1;
  return t - start;
}