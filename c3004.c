static PHP_FUNCTION(session_decode)
{
	char *str;
	int str_len;

	if (PS(session_status) == php_session_none) {
		RETURN_FALSE;
	}

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &str, &str_len) == FAILURE) {
		return;
	}

	php_session_decode(str, str_len TSRMLS_CC);

	RETURN_TRUE;
}