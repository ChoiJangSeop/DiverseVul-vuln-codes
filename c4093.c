PHP_FUNCTION(header_register_callback)
{
	zval *callback_func;
	char *callback_name;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &callback_func) == FAILURE) {
		return;
	}
	
	if (!zend_is_callable(callback_func, 0, &callback_name TSRMLS_CC)) {
		efree(callback_name);
		RETURN_FALSE;
	}

	efree(callback_name);

	if (SG(callback_func)) {
		zval_ptr_dtor(&SG(callback_func));
		SG(fci_cache) = empty_fcall_info_cache;
	}

	SG(callback_func) = callback_func;

	Z_ADDREF_P(SG(callback_func));

	RETURN_TRUE;
}