static void php_filter_encode_url(zval *value, const unsigned char* chars, const int char_len, int high, int low, int encode_nul)
{
	unsigned char *p;
	unsigned char tmp[256];
	unsigned char *s = (unsigned char *)chars;
	unsigned char *e = s + char_len;
	zend_string *str;

	memset(tmp, 1, sizeof(tmp)-1);

	while (s < e) {
		tmp[*s++] = '\0';
	}
/* XXX: This is not needed since these chars in the allowed list never include the high/low/null value
	if (encode_nul) {
		tmp[0] = 1;
	}
	if (high) {
		memset(tmp + 127, 1, sizeof(tmp) - 127);
	}
	if (low) {
		memset(tmp, 1, 32);
	}
*/
	str = zend_string_alloc(3 * Z_STRLEN_P(value), 0);
	p = (unsigned char *) ZSTR_VAL(str);
	s = (unsigned char *) Z_STRVAL_P(value);
	e = s + Z_STRLEN_P(value);

	while (s < e) {
		if (tmp[*s]) {
			*p++ = '%';
			*p++ = hexchars[(unsigned char) *s >> 4];
			*p++ = hexchars[(unsigned char) *s & 15];
		} else {
			*p++ = *s;
		}
		s++;
	}
	*p = '\0';
	ZSTR_LEN(str) = p - (unsigned char *)ZSTR_VAL(str);
	zval_ptr_dtor(value);
	ZVAL_NEW_STR(value, str);
}