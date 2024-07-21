PHP_FUNCTION(copy)
{
	char *source, *target;
	int source_len, target_len;
	zval *zcontext = NULL;
	php_stream_context *context;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss|r", &source, &source_len, &target, &target_len, &zcontext) == FAILURE) {
		return;
	}

	if (PG(safe_mode) &&(!php_checkuid(source, NULL, CHECKUID_CHECK_FILE_AND_DIR))) {
		RETURN_FALSE;
	}

	if (php_check_open_basedir(source TSRMLS_CC)) {
		RETURN_FALSE;
	}

	context = php_stream_context_from_zval(zcontext, 0);

	if (php_copy_file_ctx(source, target, 0, context TSRMLS_CC) == SUCCESS) {
		RETURN_TRUE;
	} else {
		RETURN_FALSE;
	}
}