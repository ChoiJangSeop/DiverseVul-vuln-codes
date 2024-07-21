SPL_METHOD(Array, count)
{
	long count;
	spl_array_object *intern = (spl_array_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
	
	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}

	spl_array_object_count_elements_helper(intern, &count TSRMLS_CC);

	RETURN_LONG(count);
} /* }}} */