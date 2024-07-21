utf16be_mbc_to_code(const UChar* p, const UChar* end ARG_UNUSED)
{
  OnigCodePoint code;

  if (UTF16_IS_SURROGATE_FIRST(*p)) {
    code = ((((p[0] - 0xd8) << 2) + ((p[1] & 0xc0) >> 6) + 1) << 16)
         + ((((p[1] & 0x3f) << 2) + (p[2] - 0xdc)) << 8)
         + p[3];
  }
  else {
    code = p[0] * 256 + p[1];
  }
  return code;
}