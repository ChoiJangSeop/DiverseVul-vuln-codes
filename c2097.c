PHP_FUNCTION(rename)
{
	char *old_name, *new_name;
	int old_name_len, new_name_len;
	zval *zcontext = NULL;
	php_stream_wrapper *wrapper;
	php_stream_context *context;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss|r", &old_name, &old_name_len, &new_name, &new_name_len, &zcontext) == FAILURE) {
		RETURN_FALSE;
	}

	wrapper = php_stream_locate_url_wrapper(old_name, NULL, 0 TSRMLS_CC);

	if (!wrapper || !wrapper->wops) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to locate stream wrapper");
		RETURN_FALSE;
	}

	if (!wrapper->wops->rename) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s wrapper does not support renaming", wrapper->wops->label ? wrapper->wops->label : "Source");
		RETURN_FALSE;
	}

	if (wrapper != php_stream_locate_url_wrapper(new_name, NULL, 0 TSRMLS_CC)) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Cannot rename a file across wrapper types");
		RETURN_FALSE;
	}

	context = php_stream_context_from_zval(zcontext, 0);

	RETURN_BOOL(wrapper->wops->rename(wrapper, old_name, new_name, 0, context TSRMLS_CC));
}