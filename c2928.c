SPL_METHOD(SplObjectStorage, key)
{
	spl_SplObjectStorage *intern = (spl_SplObjectStorage*)zend_object_store_get_object(getThis() TSRMLS_CC);
	
	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}
	
	RETURN_LONG(intern->index);
} /* }}} */