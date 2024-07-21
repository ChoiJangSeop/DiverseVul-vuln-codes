utf16le_mbc_to_code(const UChar* p, const UChar* end ARG_UNUSED)
{
  OnigCodePoint code;
  UChar c0 = *p;
  UChar c1 = *(p+1);

  if (UTF16_IS_SURROGATE_FIRST(c1)) {
    code = ((((c1 - 0xd8) << 2) + ((c0  & 0xc0) >> 6) + 1) << 16)
         + ((((c0 & 0x3f) << 2) + (p[3] - 0xdc)) << 8)
         + p[2];
  }
  else {
    code = c1 * 256 + p[0];
  }
  return code;
}