utf32be_mbc_to_code(const UChar* p, const UChar* end ARG_UNUSED)
{
  return (OnigCodePoint )(((p[0] * 256 + p[1]) * 256 + p[2]) * 256 + p[3]);
}