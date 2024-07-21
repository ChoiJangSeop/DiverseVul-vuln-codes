PHP_FUNCTION(symlink)
{
	char *topath, *frompath;
	int topath_len, frompath_len;
	int ret;
	char source_p[MAXPATHLEN];
	char dest_p[MAXPATHLEN];
	char dirname[MAXPATHLEN];
	size_t len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss", &topath, &topath_len, &frompath, &frompath_len) == FAILURE) {
		return;
	}
	
	if (!expand_filepath(frompath, source_p TSRMLS_CC)) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "No such file or directory");
		RETURN_FALSE;
	}

	memcpy(dirname, source_p, sizeof(source_p));
	len = php_dirname(dirname, strlen(dirname));

	if (!expand_filepath_ex(topath, dest_p, dirname, len TSRMLS_CC)) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "No such file or directory");
		RETURN_FALSE;
	}

	if (php_stream_locate_url_wrapper(source_p, NULL, STREAM_LOCATE_WRAPPERS_ONLY TSRMLS_CC) ||
		php_stream_locate_url_wrapper(dest_p, NULL, STREAM_LOCATE_WRAPPERS_ONLY TSRMLS_CC) ) 
	{
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to symlink to a URL");
		RETURN_FALSE;
	}

	if (PG(safe_mode) && !php_checkuid(dest_p, NULL, CHECKUID_CHECK_FILE_AND_DIR)) {
		RETURN_FALSE;
	}

	if (PG(safe_mode) && !php_checkuid(source_p, NULL, CHECKUID_CHECK_FILE_AND_DIR)) {
		RETURN_FALSE;
	}

	if (php_check_open_basedir(dest_p TSRMLS_CC)) {
		RETURN_FALSE;
	}

	if (php_check_open_basedir(source_p TSRMLS_CC)) {
		RETURN_FALSE;
	}

	/* For the source, an expanded path must be used (in ZTS an other thread could have changed the CWD).
	 * For the target the exact string given by the user must be used, relative or not, existing or not.
	 * The target is relative to the link itself, not to the CWD. */
	ret = symlink(topath, source_p);

	if (ret == -1) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", strerror(errno));
		RETURN_FALSE;
	}

	RETURN_TRUE;
}