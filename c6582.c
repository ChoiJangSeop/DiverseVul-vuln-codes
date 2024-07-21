utf8_str (const gchar *utf8,
          gchar       *buf)
{
  char_str (g_utf8_get_char (utf8), buf);
  return buf;
}