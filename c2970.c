SPL_METHOD(Array, current)
{
	zval *object = getThis();
	spl_array_object *intern = (spl_array_object*)zend_object_store_get_object(object TSRMLS_CC);
	zval **entry;
	HashTable *aht = spl_array_get_hash_table(intern, 0 TSRMLS_CC);
	
	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}

	if (spl_array_object_verify_pos(intern, aht TSRMLS_CC) == FAILURE) {
		return;
	}

	if (zend_hash_get_current_data_ex(aht, (void **) &entry, &intern->pos) == FAILURE) {
		return;
	}
	RETVAL_ZVAL(*entry, 1, 0);
}