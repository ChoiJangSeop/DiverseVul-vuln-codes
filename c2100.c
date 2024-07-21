PHP_FUNCTION(mkdir)
{
	char *dir;
	int dir_len;
	zval *zcontext = NULL;
	long mode = 0777;
	zend_bool recursive = 0;
	php_stream_context *context;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|lbr", &dir, &dir_len, &mode, &recursive, &zcontext) == FAILURE) {
		RETURN_FALSE;
	}

	context = php_stream_context_from_zval(zcontext, 0);

	RETURN_BOOL(php_stream_mkdir(dir, mode, (recursive ? PHP_STREAM_MKDIR_RECURSIVE : 0) | REPORT_ERRORS, context));
}