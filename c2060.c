PHP_FUNCTION(tempnam)
{
	char *dir, *prefix;
	int dir_len, prefix_len;
	size_t p_len;
	char *opened_path;
	char *p;
	int fd;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss", &dir, &dir_len, &prefix, &prefix_len) == FAILURE) {
		return;
	}

	if (PG(safe_mode) &&(!php_checkuid(dir, NULL, CHECKUID_CHECK_FILE_AND_DIR))) {
		RETURN_FALSE;
	}

	if (php_check_open_basedir(dir TSRMLS_CC)) {
		RETURN_FALSE;
	}

	php_basename(prefix, prefix_len, NULL, 0, &p, &p_len TSRMLS_CC);
	if (p_len > 64) {
		p[63] = '\0';
	}
	
	RETVAL_FALSE;

	if ((fd = php_open_temporary_fd(dir, p, &opened_path TSRMLS_CC)) >= 0) {
		close(fd);
		RETVAL_STRING(opened_path, 0);
	}
	efree(p);
}