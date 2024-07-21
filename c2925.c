SPL_METHOD(MultipleIterator, rewind)
{
	spl_SplObjectStorage        *intern;
	spl_SplObjectStorageElement *element;
	zval                        *it;

	intern = (spl_SplObjectStorage*)zend_object_store_get_object(getThis() TSRMLS_CC);
	
	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}

	zend_hash_internal_pointer_reset_ex(&intern->storage, &intern->pos);
	while (zend_hash_get_current_data_ex(&intern->storage, (void**)&element, &intern->pos) == SUCCESS && !EG(exception)) {
		it = element->obj;
		zend_call_method_with_0_params(&it, Z_OBJCE_P(it), &Z_OBJCE_P(it)->iterator_funcs.zf_rewind, "rewind", NULL);
		zend_hash_move_forward_ex(&intern->storage, &intern->pos);
	}
}