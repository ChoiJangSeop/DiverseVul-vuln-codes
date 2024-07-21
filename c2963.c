SPL_METHOD(Array, getArrayCopy)
{
	zval *object = getThis(), *tmp;
	spl_array_object *intern = (spl_array_object*)zend_object_store_get_object(object TSRMLS_CC);
    
    array_init(return_value);
	zend_hash_copy(HASH_OF(return_value), spl_array_get_hash_table(intern, 0 TSRMLS_CC), (copy_ctor_func_t) zval_add_ref, &tmp, sizeof(zval*));
} /* }}} */