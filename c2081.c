PHP_FUNCTION(readfile)
{
	char *filename;
	int filename_len;
	int size = 0;
	zend_bool use_include_path = 0;
	zval *zcontext = NULL;
	php_stream *stream;
	php_stream_context *context = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|br!", &filename, &filename_len, &use_include_path, &zcontext) == FAILURE) {
		RETURN_FALSE;
	}

	context = php_stream_context_from_zval(zcontext, 0);

	stream = php_stream_open_wrapper_ex(filename, "rb", (use_include_path ? USE_PATH : 0) | ENFORCE_SAFE_MODE | REPORT_ERRORS, NULL, context);
	if (stream) {
		size = php_stream_passthru(stream);
		php_stream_close(stream);
		RETURN_LONG(size);
	}

	RETURN_FALSE;
}