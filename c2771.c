PHP_FUNCTION(chroot)
{
	char *str;
	int ret, str_len;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &str, &str_len) == FAILURE) {
		RETURN_FALSE;
	}
	
	ret = chroot(str);
	if (ret != 0) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s (errno %d)", strerror(errno), errno);
		RETURN_FALSE;
	}

	php_clear_stat_cache(1, NULL, 0 TSRMLS_CC);
	
	ret = chdir("/");
	
	if (ret != 0) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s (errno %d)", strerror(errno), errno);
		RETURN_FALSE;
	}

	RETURN_TRUE;
}