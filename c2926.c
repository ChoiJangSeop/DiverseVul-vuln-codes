SPL_METHOD(SplObjectStorage, setInfo)
{
	spl_SplObjectStorageElement *element;
	spl_SplObjectStorage *intern = (spl_SplObjectStorage*)zend_object_store_get_object(getThis() TSRMLS_CC);
	zval *inf;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &inf) == FAILURE) {
		return;
	}

	if (zend_hash_get_current_data_ex(&intern->storage, (void**)&element, &intern->pos) == FAILURE) {
		return;
	}
	zval_ptr_dtor(&element->inf);
	element->inf = inf;
	Z_ADDREF_P(inf);
} /* }}} */