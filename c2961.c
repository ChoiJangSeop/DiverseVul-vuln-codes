SPL_METHOD(Array, setFlags)
{
	zval *object = getThis();
	spl_array_object *intern = (spl_array_object*)zend_object_store_get_object(object TSRMLS_CC);
	long ar_flags = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &ar_flags) == FAILURE) {
		return;
	}
	
	intern->ar_flags = (intern->ar_flags & SPL_ARRAY_INT_MASK) | (ar_flags & ~SPL_ARRAY_INT_MASK);
}