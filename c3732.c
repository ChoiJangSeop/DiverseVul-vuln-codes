PHP_FUNCTION(grapheme_strpos)
{
	unsigned char *haystack, *needle;
	int haystack_len, needle_len;
	unsigned char *found;
	long loffset = 0;
	int32_t offset = 0;
	int ret_pos;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss|l", (char **)&haystack, &haystack_len, (char **)&needle, &needle_len, &loffset) == FAILURE) {

		intl_error_set( NULL, U_ILLEGAL_ARGUMENT_ERROR,
			 "grapheme_strpos: unable to parse input param", 0 TSRMLS_CC );

		RETURN_FALSE;
	}

	if ( OUTSIDE_STRING(loffset, haystack_len) ) {

		intl_error_set( NULL, U_ILLEGAL_ARGUMENT_ERROR, "grapheme_strpos: Offset not contained in string", 1 TSRMLS_CC );

		RETURN_FALSE;
	}

	/* we checked that it will fit: */
	offset = (int32_t) loffset;

	/* the offset is 'grapheme count offset' so it still might be invalid - we'll check it later */

	if (needle_len == 0) {

		intl_error_set( NULL, U_ILLEGAL_ARGUMENT_ERROR, "grapheme_strpos: Empty delimiter", 1 TSRMLS_CC );

		RETURN_FALSE;
	}


	/* quick check to see if the string might be there
	 * I realize that 'offset' is 'grapheme count offset' but will work in spite of that
	*/
	found = (unsigned char *)php_memnstr((char *)haystack + offset, (char *)needle, needle_len, (char *)haystack + haystack_len);

	/* if it isn't there the we are done */
	if (!found) {
		RETURN_FALSE;
	}

	/* if it is there, and if the haystack is ascii, we are all done */
	if ( grapheme_ascii_check(haystack, haystack_len) >= 0 ) {

		RETURN_LONG(found - haystack);
	}

	/* do utf16 part of the strpos */
	ret_pos = grapheme_strpos_utf16(haystack, haystack_len, needle, needle_len, offset, NULL, 0 /* fIgnoreCase */, 0 /* last */ TSRMLS_CC );

	if ( ret_pos >= 0 ) {
		RETURN_LONG(ret_pos);
	} else {
		RETURN_FALSE;
	}

}