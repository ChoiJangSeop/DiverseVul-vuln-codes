static int php_session_destroy(TSRMLS_D) /* {{{ */
{
	int retval = SUCCESS;

	if (PS(session_status) != php_session_active) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Trying to destroy uninitialized session");
		return FAILURE;
	}

	if (PS(mod)->s_destroy(&PS(mod_data), PS(id) TSRMLS_CC) == FAILURE) {
		retval = FAILURE;
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Session object destruction failed");
	}

	php_rshutdown_session_globals(TSRMLS_C);
	php_rinit_session_globals(TSRMLS_C);

	return retval;
}