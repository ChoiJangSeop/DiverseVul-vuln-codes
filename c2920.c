SPL_METHOD(SplObjectStorage, getInfo)
{
	spl_SplObjectStorageElement *element;
	spl_SplObjectStorage *intern = (spl_SplObjectStorage*)zend_object_store_get_object(getThis() TSRMLS_CC);

	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}
	
	if (zend_hash_get_current_data_ex(&intern->storage, (void**)&element, &intern->pos) == FAILURE) {
		return;
	}
	RETVAL_ZVAL(element->inf, 1, 0);
} /* }}} */