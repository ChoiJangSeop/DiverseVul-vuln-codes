static PHP_FUNCTION(session_regenerate_id)
{
	zend_bool del_ses = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|b", &del_ses) == FAILURE) {
		return;
	}

	if (SG(headers_sent) && PS(use_cookies)) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Cannot regenerate session id - headers already sent");
		RETURN_FALSE;
	}

	if (PS(session_status) == php_session_active) {
		if (PS(id)) {
			if (del_ses && PS(mod)->s_destroy(&PS(mod_data), PS(id) TSRMLS_CC) == FAILURE) {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "Session object destruction failed");
				RETURN_FALSE;
			}
			efree(PS(id));
			PS(id) = NULL;
		}

		PS(id) = PS(mod)->s_create_sid(&PS(mod_data), NULL TSRMLS_CC);

		PS(send_cookie) = 1;
		php_session_reset_id(TSRMLS_C);

		RETURN_TRUE;
	}
	RETURN_FALSE;
}