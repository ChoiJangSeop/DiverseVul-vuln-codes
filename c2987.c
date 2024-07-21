PHP_FUNCTION(unserialize)
{
	char *buf = NULL;
	int buf_len;
	const unsigned char *p;
	php_unserialize_data_t var_hash;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &buf, &buf_len) == FAILURE) {
		RETURN_FALSE;
	}

	if (buf_len == 0) {
		RETURN_FALSE;
	}

	p = (const unsigned char*) buf;
	PHP_VAR_UNSERIALIZE_INIT(var_hash);
	if (!php_var_unserialize(&return_value, &p, p + buf_len, &var_hash TSRMLS_CC)) {
		PHP_VAR_UNSERIALIZE_DESTROY(var_hash);
		zval_dtor(return_value);
		if (!EG(exception)) {
			php_error_docref(NULL TSRMLS_CC, E_NOTICE, "Error at offset %ld of %d bytes", (long)((char*)p - buf), buf_len);
		}
		RETURN_FALSE;
	}
	PHP_VAR_UNSERIALIZE_DESTROY(var_hash);
}