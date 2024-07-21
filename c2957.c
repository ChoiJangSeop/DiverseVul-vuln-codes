SPL_METHOD(Array, hasChildren)
{
	zval *object = getThis(), **entry;
	spl_array_object *intern = (spl_array_object*)zend_object_store_get_object(object TSRMLS_CC);
	HashTable *aht = spl_array_get_hash_table(intern, 0 TSRMLS_CC);
	
	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}

	if (spl_array_object_verify_pos(intern, aht TSRMLS_CC) == FAILURE) {
		RETURN_FALSE;
	}

	if (zend_hash_get_current_data_ex(aht, (void **) &entry, &intern->pos) == FAILURE) {
		RETURN_FALSE;
	}

	RETURN_BOOL(Z_TYPE_PP(entry) == IS_ARRAY || (Z_TYPE_PP(entry) == IS_OBJECT && (intern->ar_flags & SPL_ARRAY_CHILD_ARRAYS_ONLY) == 0));
}