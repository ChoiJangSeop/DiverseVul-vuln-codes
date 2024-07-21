static PHP_FUNCTION(bzread)
{
	zval *bz;
	long len = 1024;
	php_stream *stream;

	if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r|l", &bz, &len)) {
		RETURN_FALSE;
	}
	
	php_stream_from_zval(stream, &bz);

	if ((len + 1) < 1) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "length may not be negative");
		RETURN_FALSE;
	}

	Z_STRVAL_P(return_value) = emalloc(len + 1);
	Z_STRLEN_P(return_value) = php_stream_read(stream, Z_STRVAL_P(return_value), len);
	
	if (Z_STRLEN_P(return_value) < 0) {
		efree(Z_STRVAL_P(return_value));
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "could not read valid bz2 data from stream");
		RETURN_FALSE;		
	}
	
	Z_STRVAL_P(return_value)[Z_STRLEN_P(return_value)] = 0;
	Z_TYPE_P(return_value) = IS_STRING;
}