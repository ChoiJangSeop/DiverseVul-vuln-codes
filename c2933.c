SPL_METHOD(MultipleIterator, key)
{
	spl_SplObjectStorage        *intern;
	intern = (spl_SplObjectStorage*)zend_object_store_get_object(getThis() TSRMLS_CC);
	
	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}

	spl_multiple_iterator_get_all(intern, SPL_MULTIPLE_ITERATOR_GET_ALL_KEY, return_value TSRMLS_CC);
}