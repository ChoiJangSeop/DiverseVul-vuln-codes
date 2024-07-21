label (const uint8_t * src, size_t srclen, uint8_t * dst, size_t * dstlen,
       int flags)
{
  size_t plen;
  uint32_t *p;
  int rc;
  size_t tmpl;

  if (_idn2_ascii_p (src, srclen))
    {
      if (flags & IDN2_ALABEL_ROUNDTRIP)
	/* FIXME implement this MAY:

	   If the input to this procedure appears to be an A-label
	   (i.e., it starts in "xn--", interpreted
	   case-insensitively), the lookup application MAY attempt to
	   convert it to a U-label, first ensuring that the A-label is
	   entirely in lowercase (converting it to lowercase if
	   necessary), and apply the tests of Section 5.4 and the
	   conversion of Section 5.5 to that form. */
	return IDN2_INVALID_FLAGS;

      if (srclen > IDN2_LABEL_MAX_LENGTH)
	return IDN2_TOO_BIG_LABEL;
      if (srclen > *dstlen)
	return IDN2_TOO_BIG_DOMAIN;

      memcpy (dst, src, srclen);
      *dstlen = srclen;
      return IDN2_OK;
    }

  rc = _idn2_u8_to_u32_nfc (src, srclen, &p, &plen, flags & IDN2_NFC_INPUT);
  if (rc != IDN2_OK)
    return rc;

  if (!(flags & IDN2_TRANSITIONAL))
    {
      rc = _idn2_label_test(
	TEST_NFC |
	TEST_2HYPHEN |
	TEST_LEADING_COMBINING |
	TEST_DISALLOWED |
	TEST_CONTEXTJ_RULE |
	TEST_CONTEXTO_WITH_RULE |
	TEST_UNASSIGNED | TEST_BIDI |
	((flags & IDN2_NONTRANSITIONAL) ? TEST_NONTRANSITIONAL : 0) |
	((flags & IDN2_USE_STD3_ASCII_RULES) ? 0 : TEST_ALLOW_STD3_DISALLOWED),
	p, plen);

      if (rc != IDN2_OK)
	{
	  free(p);
	  return rc;
	}
    }

  dst[0] = 'x';
  dst[1] = 'n';
  dst[2] = '-';
  dst[3] = '-';

  tmpl = *dstlen - 4;
  rc = _idn2_punycode_encode (plen, p, &tmpl, (char *) dst + 4);
  free (p);
  if (rc != IDN2_OK)
    return rc;

  *dstlen = 4 + tmpl;

  return IDN2_OK;
}