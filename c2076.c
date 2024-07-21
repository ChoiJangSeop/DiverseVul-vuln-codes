PHP_FUNCTION(posix_mknod)
{
	char *path;
	int path_len;
	long mode;
	long major = 0, minor = 0;
	int result;
	dev_t php_dev;

	php_dev = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sl|ll", &path, &path_len,
			&mode, &major, &minor) == FAILURE) {
		RETURN_FALSE;
	}

	if (php_check_open_basedir_ex(path, 0 TSRMLS_CC) ||
			(PG(safe_mode) && (!php_checkuid(path, NULL, CHECKUID_ALLOW_ONLY_DIR)))) {
		RETURN_FALSE;
	}

	if ((mode & S_IFCHR) || (mode & S_IFBLK)) {
		if (ZEND_NUM_ARGS() == 2) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "For S_IFCHR and S_IFBLK you need to pass a major device kernel identifier");
			RETURN_FALSE;
		}
		if (major == 0) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING,
				"Expects argument 3 to be non-zero for POSIX_S_IFCHR and POSIX_S_IFBLK");
			RETURN_FALSE;
		} else {
#if defined(HAVE_MAKEDEV) || defined(makedev)
			php_dev = makedev(major, minor);
#else
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Cannot create a block or character device, creating a normal file instead");
#endif
		}
	}

	result = mknod(path, mode, php_dev);
	if (result < 0) {
		POSIX_G(last_error) = errno;
		RETURN_FALSE;
	}

	RETURN_TRUE;
}