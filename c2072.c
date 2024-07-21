PHP_FUNCTION(fnmatch)
{
	char *pattern, *filename;
	int pattern_len, filename_len;
	long flags = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss|l", &pattern, &pattern_len, &filename, &filename_len, &flags) == FAILURE) {
		return;
	}

	if (filename_len >= MAXPATHLEN) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Filename exceeds the maximum allowed length of %d characters", MAXPATHLEN);
		RETURN_FALSE;
	}
	if (pattern_len >= MAXPATHLEN) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Pattern exceeds the maximum allowed length of %d characters", MAXPATHLEN);
		RETURN_FALSE;
	}

	RETURN_BOOL( ! fnmatch( pattern, filename, flags ));
}