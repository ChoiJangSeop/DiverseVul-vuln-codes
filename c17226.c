_nc_fmt_funcptr(char *target, const char *source, size_t size)
{
    size_t n;
    char *dst = target;
    bool leading = TRUE;

    union {
	int value;
	char bytes[sizeof(int)];
    } byteorder;

    byteorder.value = 0x1234;

    *dst++ = '0';
    *dst++ = 'x';

    for (n = 0; n < size; ++n) {
	unsigned ch = ((byteorder.bytes[0] == 0x34)
		       ? UChar(source[size - n - 1])
		       : UChar(source[n]));
	if (ch != 0 || (n + 1) >= size)
	    leading = FALSE;
	if (!leading) {
	    _nc_SPRINTF(dst, _nc_SLIMIT(TR_FUNC_LEN - (dst - target))
			"%02x", ch & 0xff);
	    dst += 2;
	}
    }
    *dst = '\0';
    return target;
}