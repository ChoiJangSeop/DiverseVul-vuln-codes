idna_to_unicode_8z8z (const char *input, char **output, int flags)
{
  uint32_t *ucs4;
  int rc;

  rc = idna_to_unicode_8z4z (input, &ucs4, flags);
  *output = stringprep_ucs4_to_utf8 (ucs4, -1, NULL, NULL);
  free (ucs4);

  if (!*output)
    return IDNA_ICONV_ERROR;

  return rc;
}