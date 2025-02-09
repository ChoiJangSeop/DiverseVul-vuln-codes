PHP_FUNCTION(grapheme_substr)
{
	unsigned char *str, *sub_str;
	UChar *ustr;
	int str_len, sub_str_len, ustr_len;
	long lstart = 0, length = 0;
	int32_t start = 0;
	int iter_val;
	UErrorCode status;
	unsigned char u_break_iterator_buffer[U_BRK_SAFECLONE_BUFFERSIZE];
	UBreakIterator* bi = NULL;
	int sub_str_start_pos, sub_str_end_pos;
	int32_t (*iter_func)(UBreakIterator *);

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sl|l", (char **)&str, &str_len, &lstart, &length) == FAILURE) {

		intl_error_set( NULL, U_ILLEGAL_ARGUMENT_ERROR,
			 "grapheme_substr: unable to parse input param", 0 TSRMLS_CC );

		RETURN_FALSE;
	}

	if ( OUTSIDE_STRING(lstart, str_len) ) {

		intl_error_set( NULL, U_ILLEGAL_ARGUMENT_ERROR, "grapheme_substr: start not contained in string", 1 TSRMLS_CC );

		RETURN_FALSE;
	}

	/* we checked that it will fit: */
	start = (int32_t) lstart;

	/* the offset is 'grapheme count offset' so it still might be invalid - we'll check it later */

	if ( grapheme_ascii_check(str, str_len) >= 0 ) {
		grapheme_substr_ascii((char *)str, str_len, start, length, ZEND_NUM_ARGS(), (char **) &sub_str, &sub_str_len);

		if ( NULL == sub_str ) {
			intl_error_set( NULL, U_ILLEGAL_ARGUMENT_ERROR, "grapheme_substr: invalid parameters", 1 TSRMLS_CC );
			RETURN_FALSE;
		}

		RETURN_STRINGL(((char *)sub_str), sub_str_len, 1);
	}

	ustr = NULL;
	ustr_len = 0;
	status = U_ZERO_ERROR;
	intl_convert_utf8_to_utf16(&ustr, &ustr_len, (char *)str, str_len, &status);

	if ( U_FAILURE( status ) ) {
		/* Set global error code. */
		intl_error_set_code( NULL, status TSRMLS_CC );

		/* Set error messages. */
		intl_error_set_custom_msg( NULL, "Error converting input string to UTF-16", 0 TSRMLS_CC );
		if (ustr) {
			efree( ustr );
		}
		RETURN_FALSE;
	}

	bi = grapheme_get_break_iterator((void*)u_break_iterator_buffer, &status TSRMLS_CC );

	if( U_FAILURE(status) ) {
		RETURN_FALSE;
	}

	ubrk_setText(bi, ustr, ustr_len,	&status);

	if ( start < 0 ) {
		iter_func = ubrk_previous;
		ubrk_last(bi);
		iter_val = 1;
	}
	else {
		iter_func = ubrk_next;
		iter_val = -1;
	}

	sub_str_start_pos = 0;

	while ( start ) {
		sub_str_start_pos = iter_func(bi);

		if ( UBRK_DONE == sub_str_start_pos ) {
			break;
		}

		start += iter_val;
	}

	if ( 0 != start || sub_str_start_pos >= ustr_len ) {

		intl_error_set( NULL, U_ILLEGAL_ARGUMENT_ERROR, "grapheme_substr: start not contained in string", 1 TSRMLS_CC );

		if (ustr) {
			efree(ustr);
		}
		ubrk_close(bi);
		RETURN_FALSE;
	}

	if (ZEND_NUM_ARGS() <= 2) {

		/* no length supplied, return the rest of the string */

		sub_str = NULL;
		sub_str_len = 0;
		status = U_ZERO_ERROR;
		intl_convert_utf16_to_utf8((char **)&sub_str, &sub_str_len, ustr + sub_str_start_pos, ustr_len - sub_str_start_pos, &status);

		if (ustr) {
			efree( ustr );
		}
		ubrk_close( bi );

		if ( U_FAILURE( status ) ) {
			/* Set global error code. */
			intl_error_set_code( NULL, status TSRMLS_CC );

			/* Set error messages. */
			intl_error_set_custom_msg( NULL, "Error converting output string to UTF-8", 0 TSRMLS_CC );

			if (sub_str) {
				efree( sub_str );
			}

			RETURN_FALSE;
		}

		/* return the allocated string, not a duplicate */
		RETURN_STRINGL(((char *)sub_str), sub_str_len, 0);
	}

	if(length == 0) {
		/* empty length - we've validated start, we can return "" now */
		if (ustr) {
			efree(ustr);
		}
		ubrk_close(bi);
		RETURN_EMPTY_STRING();		
	}

	/* find the end point of the string to return */

	if ( length < 0 ) {
		iter_func = ubrk_previous;
		ubrk_last(bi);
		iter_val = 1;
	}
	else {
		iter_func = ubrk_next;
		iter_val = -1;
	}

	sub_str_end_pos = 0;

	while ( length ) {
		sub_str_end_pos = iter_func(bi);

		if ( UBRK_DONE == sub_str_end_pos ) {
			break;
		}

		length += iter_val;
	}

	ubrk_close(bi);

	if ( UBRK_DONE == sub_str_end_pos) {
		if(length < 0) {
			intl_error_set( NULL, U_ILLEGAL_ARGUMENT_ERROR, "grapheme_substr: length not contained in string", 1 TSRMLS_CC );

			efree(ustr);
			RETURN_FALSE;
		} else {
			sub_str_end_pos = ustr_len;
		}
	}
	
	if(sub_str_start_pos > sub_str_end_pos) {
		intl_error_set( NULL, U_ILLEGAL_ARGUMENT_ERROR, "grapheme_substr: length is beyond start", 1 TSRMLS_CC );

		efree(ustr);
		RETURN_FALSE;
	}

	sub_str = NULL;
	status = U_ZERO_ERROR;
	intl_convert_utf16_to_utf8((char **)&sub_str, &sub_str_len, ustr + sub_str_start_pos, ( sub_str_end_pos - sub_str_start_pos ), &status);

	efree( ustr );

	if ( U_FAILURE( status ) ) {
		/* Set global error code. */
		intl_error_set_code( NULL, status TSRMLS_CC );

		/* Set error messages. */
		intl_error_set_custom_msg( NULL, "Error converting output string to UTF-8", 0 TSRMLS_CC );

		if ( NULL != sub_str )
			efree( sub_str );

		RETURN_FALSE;
	}

	 /* return the allocated string, not a duplicate */
	RETURN_STRINGL(((char *)sub_str), sub_str_len, 0);

}