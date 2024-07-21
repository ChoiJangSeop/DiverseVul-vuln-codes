onigenc_single_byte_code_to_mbc(OnigCodePoint code, UChar *buf, OnigEncoding enc ARG_UNUSED)
{
#ifdef RUBY
  if (code > 0xff)
    rb_raise(rb_eRangeError, "%u out of char range", code);
#endif
  *buf = (UChar )(code & 0xff);
  return 1;
}