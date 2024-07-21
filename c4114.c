static void php_bz2_error(INTERNAL_FUNCTION_PARAMETERS, int opt)
{ 
	zval         *bzp;     /* BZip2 Resource Pointer */
	php_stream   *stream;
	const char   *errstr;  /* Error string */
	int           errnum;  /* Error number */
	struct php_bz2_stream_data_t *self;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &bzp) == FAILURE) {
		return;
	}

	php_stream_from_zval(stream, &bzp);

	if (!php_stream_is(stream, PHP_STREAM_IS_BZIP2)) {
		RETURN_FALSE;
	}

	self = (struct php_bz2_stream_data_t *) stream->abstract;
	
	/* Fetch the error information */
	errstr = BZ2_bzerror(self->bz_file, &errnum);
	
	/* Determine what to return */
	switch (opt) {
		case PHP_BZ_ERRNO:
			RETURN_LONG(errnum);
			break;
		case PHP_BZ_ERRSTR:
			RETURN_STRING((char*)errstr, 1);
			break;
		case PHP_BZ_ERRBOTH:
			array_init(return_value);
		
			add_assoc_long  (return_value, "errno",  errnum);
			add_assoc_string(return_value, "errstr", (char*)errstr, 1);
			break;
	}
}