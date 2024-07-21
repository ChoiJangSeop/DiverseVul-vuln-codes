t1mac_output_ascii(char *s, int len)
{
  if (blocktyp == POST_BINARY) {
    output_current_post();
    blocktyp = POST_ASCII;
  }
  /* Mac line endings */
  if (len > 0 && s[len-1] == '\n')
    s[len-1] = '\r';
  t1mac_output_data((byte *)s, len);
  if (strncmp(s, "/FontName", 9) == 0) {
    for (s += 9; isspace(*s); s++) ;
    if (*s == '/') {
      const char *t = ++s;
      while (*t && !isspace(*t)) t++;
      free(font_name);
      font_name = (char *)malloc(t - s + 1);
      memcpy(font_name, s, t - s);
      font_name[t - s] = 0;
    }
  }
}