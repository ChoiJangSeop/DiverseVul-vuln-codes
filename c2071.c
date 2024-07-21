PHP_FUNCTION(readlink)
{
	char *link;
	int link_len;
	char buff[MAXPATHLEN];
	int ret;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &link, &link_len) == FAILURE) {
		return;
	}

	if (PG(safe_mode) && !php_checkuid(link, NULL, CHECKUID_CHECK_FILE_AND_DIR)) {
		RETURN_FALSE;
	}

	if (php_check_open_basedir(link TSRMLS_CC)) {
		RETURN_FALSE;
	}

	ret = php_sys_readlink(link, buff, MAXPATHLEN-1);

	if (ret == -1) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", strerror(errno));
		RETURN_FALSE;
	}
	/* Append NULL to the end of the string */
	buff[ret] = '\0';

	RETURN_STRING(buff, 1);
}