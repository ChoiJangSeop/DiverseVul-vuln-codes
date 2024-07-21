_PUBLIC_ codepoint_t next_codepoint_handle(struct smb_iconv_handle *ic,
				    const char *str, size_t *size)
{
	return next_codepoint_handle_ext(ic, str, CH_UNIX, size);
}