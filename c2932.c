SPL_METHOD(SplObjectStorage, count)
{
	spl_SplObjectStorage *intern = (spl_SplObjectStorage*)zend_object_store_get_object(getThis() TSRMLS_CC);
	
	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}
	
	RETURN_LONG(zend_hash_num_elements(&intern->storage));
} /* }}} */