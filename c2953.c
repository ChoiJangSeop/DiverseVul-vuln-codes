SPL_METHOD(Array, getIteratorClass)
{
	zval *object = getThis();
	spl_array_object *intern = (spl_array_object*)zend_object_store_get_object(object TSRMLS_CC);
	
	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}

	RETURN_STRING(intern->ce_get_iterator->name, 1);
}