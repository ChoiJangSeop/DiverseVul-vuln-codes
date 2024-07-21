PHP_FUNCTION(rmdir)
{
	char *dir;
	int dir_len;
	zval *zcontext = NULL;
	php_stream_context *context;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|r", &dir, &dir_len, &zcontext) == FAILURE) {
		RETURN_FALSE;
	}

	context = php_stream_context_from_zval(zcontext, 0);

	RETURN_BOOL(php_stream_rmdir(dir, REPORT_ERRORS, context));
}