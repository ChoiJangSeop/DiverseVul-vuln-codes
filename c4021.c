SPL_METHOD(SplFileObject, ftruncate)
{
	spl_filesystem_object *intern = (spl_filesystem_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
	long size;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &size) == FAILURE) {
		return;
	}

	if (!php_stream_truncate_supported(intern->u.file.stream)) {
		zend_throw_exception_ex(spl_ce_LogicException, 0 TSRMLS_CC, "Can't truncate file %s", intern->file_name);
		RETURN_FALSE;
	}
	
	RETURN_BOOL(0 == php_stream_truncate_set_size(intern->u.file.stream, size));
} /* }}} */