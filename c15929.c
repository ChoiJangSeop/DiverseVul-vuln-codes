int utf8s_to_utf16s(const u8 *s, int len, wchar_t *pwcs)
{
	u16 *op;
	int size;
	unicode_t u;

	op = pwcs;
	while (*s && len > 0) {
		if (*s & 0x80) {
			size = utf8_to_utf32(s, len, &u);
			if (size < 0)
				return -EINVAL;

			if (u >= PLANE_SIZE) {
				u -= PLANE_SIZE;
				*op++ = (wchar_t) (SURROGATE_PAIR |
						((u >> 10) & SURROGATE_BITS));
				*op++ = (wchar_t) (SURROGATE_PAIR |
						SURROGATE_LOW |
						(u & SURROGATE_BITS));
			} else {
				*op++ = (wchar_t) u;
			}
			s += size;
			len -= size;
		} else {
			*op++ = *s++;
			len--;
		}
	}
	return op - pwcs;
}