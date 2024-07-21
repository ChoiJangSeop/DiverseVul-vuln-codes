path_equal (const char *a, const char *b, gboolean relax)
{
  gboolean res;
  size_t a_len, b_len;
  char *ua, *ub;

  if (relax == FALSE)
    return str_equal (a, b, FALSE);

  if (a == NULL || b == NULL)
      return a == b;

  ua = g_uri_unescape_string (a, "/");
  ub = g_uri_unescape_string (b, "/");

  a_len = strlen (ua);
  b_len = strlen (ub);

  while (a_len > 0 && ua[a_len - 1] == '/')
    a_len--;

  while (b_len > 0 && ub[b_len - 1] == '/')
    b_len--;

  if (a_len == b_len)
    res = ! strncmp (ua, ub, a_len);
  else
    res = FALSE;

  g_free(ua);
  g_free(ub);

  return res;
}