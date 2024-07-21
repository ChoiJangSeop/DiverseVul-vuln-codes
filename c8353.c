str_lower_case_match(OnigEncoding enc, int case_fold_flag,
                     const UChar* t, const UChar* tend,
                     const UChar* p, const UChar* end)
{
  int lowlen;
  UChar *q, lowbuf[ONIGENC_MBC_CASE_FOLD_MAXLEN];

  while (t < tend) {
    lowlen = ONIGENC_MBC_CASE_FOLD(enc, case_fold_flag, &p, end, lowbuf);
    q = lowbuf;
    while (lowlen > 0) {
      if (*t++ != *q++) return 0;
      lowlen--;
    }
  }

  return 1;
}