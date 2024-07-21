PHP_FUNCTION(unlink)
{
	char *filename;
	int filename_len;
	php_stream_wrapper *wrapper;
	zval *zcontext = NULL;
	php_stream_context *context = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|r", &filename, &filename_len, &zcontext) == FAILURE) {
		RETURN_FALSE;
	}

	context = php_stream_context_from_zval(zcontext, 0);

	wrapper = php_stream_locate_url_wrapper(filename, NULL, 0 TSRMLS_CC);

	if (!wrapper || !wrapper->wops) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to locate stream wrapper");
		RETURN_FALSE;
	}

	if (!wrapper->wops->unlink) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s does not allow unlinking", wrapper->wops->label ? wrapper->wops->label : "Wrapper");
		RETURN_FALSE;
	}
	RETURN_BOOL(wrapper->wops->unlink(wrapper, filename, ENFORCE_SAFE_MODE | REPORT_ERRORS, context TSRMLS_CC));
}