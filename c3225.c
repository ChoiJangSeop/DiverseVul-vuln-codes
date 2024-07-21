_PUBLIC_ codepoint_t next_codepoint_ext(const char *str, charset_t src_charset,
					size_t *size)
{
	return next_codepoint_handle_ext(get_iconv_handle(), str,
					      src_charset, size);
}