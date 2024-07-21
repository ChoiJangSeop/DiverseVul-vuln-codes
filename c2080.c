PHP_FUNCTION(posix_access)
{
	long mode = 0;
	int filename_len, ret;
	char *filename, *path;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|l", &filename, &filename_len, &mode) == FAILURE) {
		RETURN_FALSE;
	}

	path = expand_filepath(filename, NULL TSRMLS_CC);
	if (!path) {
		POSIX_G(last_error) = EIO;
		RETURN_FALSE;
	}

	if (php_check_open_basedir_ex(path, 0 TSRMLS_CC) ||
			(PG(safe_mode) && (!php_checkuid_ex(filename, NULL, CHECKUID_CHECK_FILE_AND_DIR, CHECKUID_NO_ERRORS)))) {
		efree(path);
		POSIX_G(last_error) = EPERM;
		RETURN_FALSE;
	}

	ret = access(path, mode);
	efree(path);

	if (ret) {
		POSIX_G(last_error) = errno;
		RETURN_FALSE;
	}

	RETURN_TRUE;
}