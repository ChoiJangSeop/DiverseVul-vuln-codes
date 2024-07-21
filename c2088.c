PHP_FUNCTION(chdir)
{
	char *str;
	int ret, str_len;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &str, &str_len) == FAILURE) {
		RETURN_FALSE;
	}

	if ((PG(safe_mode) && !php_checkuid(str, NULL, CHECKUID_CHECK_FILE_AND_DIR)) || php_check_open_basedir(str TSRMLS_CC)) {
		RETURN_FALSE;
	}
	ret = VCWD_CHDIR(str);
	
	if (ret != 0) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s (errno %d)", strerror(errno), errno);
		RETURN_FALSE;
	}

	if (BG(CurrentStatFile) && !IS_ABSOLUTE_PATH(BG(CurrentStatFile), strlen(BG(CurrentStatFile)))) {
		efree(BG(CurrentStatFile));
		BG(CurrentStatFile) = NULL;
	}
	if (BG(CurrentLStatFile) && !IS_ABSOLUTE_PATH(BG(CurrentLStatFile), strlen(BG(CurrentLStatFile)))) {
		efree(BG(CurrentLStatFile));
		BG(CurrentLStatFile) = NULL;
	}

	RETURN_TRUE;
}