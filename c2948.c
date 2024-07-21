SPL_METHOD(Array, getFlags)
{
	zval *object = getThis();
	spl_array_object *intern = (spl_array_object*)zend_object_store_get_object(object TSRMLS_CC);
	
	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}
	
	RETURN_LONG(intern->ar_flags & ~SPL_ARRAY_INT_MASK);
}