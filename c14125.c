PHP_FUNCTION( msgfmt_format_message )
{
	zval       *args;
	UChar      *spattern = NULL;
	int         spattern_len = 0;
	char       *pattern = NULL;
	int         pattern_len = 0;
	const char *slocale = NULL;
	int         slocale_len = 0;
	MessageFormatter_object mf = {0};
	MessageFormatter_object *mfo = &mf;

	/* Parse parameters. */
	if( zend_parse_method_parameters( ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "ssa",
		  &slocale, &slocale_len, &pattern, &pattern_len, &args ) == FAILURE )
	{
		intl_error_set( NULL, U_ILLEGAL_ARGUMENT_ERROR,
			"msgfmt_format_message: unable to parse input params", 0 TSRMLS_CC );

		RETURN_FALSE;
	}

	msgformat_data_init(&mfo->mf_data TSRMLS_CC);

	if(pattern && pattern_len) {
		intl_convert_utf8_to_utf16(&spattern, &spattern_len, pattern, pattern_len, &INTL_DATA_ERROR_CODE(mfo));
		if( U_FAILURE(INTL_DATA_ERROR_CODE((mfo))) )
		{
			intl_error_set( NULL, U_ILLEGAL_ARGUMENT_ERROR,
				"msgfmt_format_message: error converting pattern to UTF-16", 0 TSRMLS_CC );
			RETURN_FALSE;
		}
	} else {
		spattern_len = 0;
		spattern = NULL;
	}

	if(slocale_len == 0) {
		slocale = intl_locale_get_default(TSRMLS_C);
	}

#ifdef MSG_FORMAT_QUOTE_APOS
	if(msgformat_fix_quotes(&spattern, &spattern_len, &INTL_DATA_ERROR_CODE(mfo)) != SUCCESS) {
		intl_error_set( NULL, U_INVALID_FORMAT_ERROR,
			"msgfmt_format_message: error converting pattern to quote-friendly format", 0 TSRMLS_CC );
		RETURN_FALSE;
	}
#endif

	/* Create an ICU message formatter. */
	MSG_FORMAT_OBJECT(mfo) = umsg_open(spattern, spattern_len, slocale, NULL, &INTL_DATA_ERROR_CODE(mfo));
	if(spattern && spattern_len) {
		efree(spattern);
	}
	INTL_METHOD_CHECK_STATUS(mfo, "Creating message formatter failed");

	msgfmt_do_format(mfo, args, return_value TSRMLS_CC);

	/* drop the temporary formatter */
	msgformat_data_free(&mfo->mf_data TSRMLS_CC);
}