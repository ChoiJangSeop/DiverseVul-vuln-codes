stringprep (char *in,
	    size_t maxlen,
	    Stringprep_profile_flags flags,
	    const Stringprep_profile * profile)
{
  int rc;
  char *utf8 = NULL;
  uint32_t *ucs4 = NULL;
  size_t ucs4len, maxucs4len, adducs4len = 50;

  do
    {
      uint32_t *newp;

      free (ucs4);
      ucs4 = stringprep_utf8_to_ucs4 (in, -1, &ucs4len);
      maxucs4len = ucs4len + adducs4len;
      newp = realloc (ucs4, maxucs4len * sizeof (uint32_t));
      if (!newp)
	{
	  free (ucs4);
	  return STRINGPREP_MALLOC_ERROR;
	}
      ucs4 = newp;

      rc = stringprep_4i (ucs4, &ucs4len, maxucs4len, flags, profile);
      adducs4len += 50;
    }
  while (rc == STRINGPREP_TOO_SMALL_BUFFER);
  if (rc != STRINGPREP_OK)
    {
      free (ucs4);
      return rc;
    }

  utf8 = stringprep_ucs4_to_utf8 (ucs4, ucs4len, 0, 0);
  free (ucs4);
  if (!utf8)
    return STRINGPREP_MALLOC_ERROR;

  if (strlen (utf8) >= maxlen)
    {
      free (utf8);
      return STRINGPREP_TOO_SMALL_BUFFER;
    }

  strcpy (in, utf8);		/* flawfinder: ignore */

  free (utf8);

  return STRINGPREP_OK;
}