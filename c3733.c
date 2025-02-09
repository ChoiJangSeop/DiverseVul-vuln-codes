PHP_FUNCTION(grapheme_stripos)
{
	unsigned char *haystack, *needle, *haystack_dup, *needle_dup;
	int haystack_len, needle_len;
	unsigned char *found;
	long loffset = 0;
	int32_t offset = 0;
	int ret_pos;
	int is_ascii;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss|l", (char **)&haystack, &haystack_len, (char **)&needle, &needle_len, &loffset) == FAILURE) {

		intl_error_set( NULL, U_ILLEGAL_ARGUMENT_ERROR,
			 "grapheme_stripos: unable to parse input param", 0 TSRMLS_CC );

		RETURN_FALSE;
	}

	if ( OUTSIDE_STRING(loffset, haystack_len) ) {

		intl_error_set( NULL, U_ILLEGAL_ARGUMENT_ERROR, "grapheme_stripos: Offset not contained in string", 1 TSRMLS_CC );

		RETURN_FALSE;
	}

	/* we checked that it will fit: */
	offset = (int32_t) loffset;

	/* the offset is 'grapheme count offset' so it still might be invalid - we'll check it later */

	if (needle_len == 0) {

		intl_error_set( NULL, U_ILLEGAL_ARGUMENT_ERROR, "grapheme_stripos: Empty delimiter", 1 TSRMLS_CC );

		RETURN_FALSE;
	}


	is_ascii = ( grapheme_ascii_check(haystack, haystack_len) >= 0 );

	if ( is_ascii ) {
		needle_dup = (unsigned char *)estrndup((char *)needle, needle_len);
		php_strtolower((char *)needle_dup, needle_len);
		haystack_dup = (unsigned char *)estrndup((char *)haystack, haystack_len);
		php_strtolower((char *)haystack_dup, haystack_len);

		found = (unsigned char*) php_memnstr((char *)haystack_dup + offset, (char *)needle_dup, needle_len, (char *)haystack_dup + haystack_len);

		efree(haystack_dup);
		efree(needle_dup);

		if (found) {
			RETURN_LONG(found - haystack_dup);
		}

		/* if needle was ascii too, we are all done, otherwise we need to try using Unicode to see what we get */
		if ( grapheme_ascii_check(needle, needle_len) >= 0 ) {
			RETURN_FALSE;
		}
	}

	/* do utf16 part of the strpos */
	ret_pos = grapheme_strpos_utf16(haystack, haystack_len, needle, needle_len, offset, NULL, 1 /* fIgnoreCase */, 0 /*last */ TSRMLS_CC );

	if ( ret_pos >= 0 ) {
		RETURN_LONG(ret_pos);
	} else {
		RETURN_FALSE;
	}

}