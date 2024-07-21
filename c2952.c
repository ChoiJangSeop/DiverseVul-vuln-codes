SPL_METHOD(Array, exchangeArray)
{
	zval *object = getThis(), *tmp, **array;
	spl_array_object *intern = (spl_array_object*)zend_object_store_get_object(object TSRMLS_CC);

	array_init(return_value);
	zend_hash_copy(HASH_OF(return_value), spl_array_get_hash_table(intern, 0 TSRMLS_CC), (copy_ctor_func_t) zval_add_ref, &tmp, sizeof(zval*));
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Z", &array) == FAILURE) {
		return;
	}

	spl_array_set_array(object, intern, array, 0L, 1 TSRMLS_CC);

}