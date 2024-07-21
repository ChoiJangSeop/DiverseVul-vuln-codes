SPL_METHOD(Array, key)
{
	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}
	
	spl_array_iterator_key(getThis(), return_value TSRMLS_CC);
} /* }}} */