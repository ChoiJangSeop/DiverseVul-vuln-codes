SPL_METHOD(SplObjectStorage, valid)
{
	spl_SplObjectStorage *intern = (spl_SplObjectStorage*)zend_object_store_get_object(getThis() TSRMLS_CC);
	
	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}
	
	RETURN_BOOL(zend_hash_has_more_elements_ex(&intern->storage, &intern->pos) == SUCCESS);
} /* }}} */