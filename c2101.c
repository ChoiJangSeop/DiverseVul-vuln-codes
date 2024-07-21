PHP_NAMED_FUNCTION(php_if_fopen)
{
	char *filename, *mode;
	int filename_len, mode_len;
	zend_bool use_include_path = 0;
	zval *zcontext = NULL;
	php_stream *stream;
	php_stream_context *context = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss|br", &filename, &filename_len, &mode, &mode_len, &use_include_path, &zcontext) == FAILURE) {
		RETURN_FALSE;
	}

	context = php_stream_context_from_zval(zcontext, 0);

	stream = php_stream_open_wrapper_ex(filename, mode, (use_include_path ? USE_PATH : 0) | ENFORCE_SAFE_MODE | REPORT_ERRORS, NULL, context);

	if (stream == NULL) {
		RETURN_FALSE;
	}

	php_stream_to_zval(stream, return_value);
}