PHP_FUNCTION(link)
{
	char *topath, *frompath;
	int topath_len, frompath_len;
	int ret;
	char source_p[MAXPATHLEN];
	char dest_p[MAXPATHLEN];

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss", &topath, &topath_len, &frompath, &frompath_len) == FAILURE) {
		return;
	}

	if (!expand_filepath(frompath, source_p TSRMLS_CC) || !expand_filepath(topath, dest_p TSRMLS_CC)) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "No such file or directory");
		RETURN_FALSE;
	}

	if (php_stream_locate_url_wrapper(source_p, NULL, STREAM_LOCATE_WRAPPERS_ONLY TSRMLS_CC) ||
		php_stream_locate_url_wrapper(dest_p, NULL, STREAM_LOCATE_WRAPPERS_ONLY TSRMLS_CC) ) 
	{
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to link to a URL");
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

#ifndef ZTS
	ret = link(topath, frompath);
#else 
	ret = link(dest_p, source_p);	
#endif	
	if (ret == -1) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", strerror(errno));
		RETURN_FALSE;
	}

	RETURN_TRUE;
}