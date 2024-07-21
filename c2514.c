idna_to_unicode_8zlz (const char *input, char **output, int flags)
{
  char *utf8;
  int rc;

  rc = idna_to_unicode_8z8z (input, &utf8, flags);
  *output = stringprep_utf8_to_locale (utf8);
  free (utf8);

  if (!*output)
    return IDNA_ICONV_ERROR;

  return rc;
}