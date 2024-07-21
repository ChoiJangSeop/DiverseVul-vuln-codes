SPL_METHOD(SplObjectStorage, rewind)
{
	spl_SplObjectStorage *intern = (spl_SplObjectStorage*)zend_object_store_get_object(getThis() TSRMLS_CC);
	
	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}
	
	zend_hash_internal_pointer_reset_ex(&intern->storage, &intern->pos);
	intern->index = 0;
} /* }}} */