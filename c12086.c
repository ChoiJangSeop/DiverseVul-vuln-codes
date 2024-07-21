ephy_string_shorten (char  *str,
                     gsize  target_length)
{
  char *new_str;
  glong actual_length;
  gulong bytes;

  g_assert (target_length > 0);

  if (!str)
    return NULL;

  /* FIXME: this function is a big mess. While it is utf-8 safe now,
   * it can still split a sequence of combining characters.
   */
  actual_length = g_utf8_strlen (str, -1);

  /* if the string is already short enough, or if it's too short for
   * us to shorten it, return a new copy */
  if ((gsize)actual_length <= target_length)
    return str;

  /* create string */
  bytes = GPOINTER_TO_UINT (g_utf8_offset_to_pointer (str, target_length - 1) - str);

  /* +1 for ellipsis, +1 for trailing NUL */
  new_str = g_new (gchar, bytes + 1 + 1);

  strncpy (new_str, str, bytes);
  strcat (new_str, "…");

  g_free (str);

  return new_str;
}