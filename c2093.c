PHP_FUNCTION(posix_mkfifo)
{
	char *path;
	int path_len;
	long mode;
	int     result;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sl", &path, &path_len, &mode) == FAILURE) {
		RETURN_FALSE;
	}

	if (php_check_open_basedir_ex(path, 0 TSRMLS_CC) ||
			(PG(safe_mode) && (!php_checkuid(path, NULL, CHECKUID_ALLOW_ONLY_DIR)))) {
		RETURN_FALSE;
	}

	result = mkfifo(path, mode);
	if (result < 0) {
		POSIX_G(last_error) = errno;
		RETURN_FALSE;
	}

	RETURN_TRUE;
}