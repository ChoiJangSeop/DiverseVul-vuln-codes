SPL_METHOD(Array, getIterator)
{
	zval *object = getThis();
	spl_array_object *intern = (spl_array_object*)zend_object_store_get_object(object TSRMLS_CC);
	spl_array_object *iterator;
	HashTable *aht = spl_array_get_hash_table(intern, 0 TSRMLS_CC);
	
	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}

	if (!aht) {
		php_error_docref(NULL TSRMLS_CC, E_NOTICE, "Array was modified outside object and is no longer an array");
		return;
	}

	return_value->type = IS_OBJECT;
	return_value->value.obj = spl_array_object_new_ex(intern->ce_get_iterator, &iterator, object, 0 TSRMLS_CC);
	Z_SET_REFCOUNT_P(return_value, 1);
	Z_SET_ISREF_P(return_value);
}