void json_append_escaped_data(string_t *dest, const unsigned char *src, size_t size)
{
	size_t i;
	int bytes = 0;
	unichar_t chr;

	for (i = 0; i < size;) {
		bytes = uni_utf8_get_char_n(src+i, size-i, &chr);
		/* refuse to add invalid data */
		i_assert(bytes > 0 && uni_is_valid_ucs4(chr));
		json_append_escaped_ucs4(dest, chr);
		i += bytes;
	}
}