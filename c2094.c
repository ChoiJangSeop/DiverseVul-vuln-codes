PHP_FUNCTION(realpath)
{
	char *filename;
	int filename_len;
	char resolved_path_buff[MAXPATHLEN];

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &filename, &filename_len) == FAILURE) {
		return;
	}

	if (VCWD_REALPATH(filename, resolved_path_buff)) {
		if (PG(safe_mode) && (!php_checkuid(resolved_path_buff, NULL, CHECKUID_CHECK_FILE_AND_DIR))) {
			RETURN_FALSE;
		}

		if (php_check_open_basedir(resolved_path_buff TSRMLS_CC)) {
			RETURN_FALSE;
		}

#ifdef ZTS
		if (VCWD_ACCESS(resolved_path_buff, F_OK)) {
			RETURN_FALSE;
		}
#endif
		RETURN_STRING(resolved_path_buff, 1);
	} else {
		RETURN_FALSE;
	}
}