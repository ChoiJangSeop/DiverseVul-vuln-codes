PHP_FUNCTION(sqlite_factory)
{
	long mode = 0666;
	char *filename, *fullpath = NULL;
	int filename_len;
	zval *errmsg = NULL;
	zend_error_handling error_handling;

	zend_replace_error_handling(EH_THROW, sqlite_ce_exception, &error_handling TSRMLS_CC);
	if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|lz/",
				&filename, &filename_len, &mode, &errmsg)) {
		zend_restore_error_handling(&error_handling TSRMLS_CC);
		RETURN_NULL();
	}
	if (errmsg) {
		zval_dtor(errmsg);
		ZVAL_NULL(errmsg);
	}

	if (strncmp(filename, ":memory:", sizeof(":memory:") - 1)) {
		/* resolve the fully-qualified path name to use as the hash key */
		if (!(fullpath = expand_filepath(filename, NULL TSRMLS_CC))) {
			zend_restore_error_handling(&error_handling TSRMLS_CC);
			RETURN_NULL();
		}

		if ((PG(safe_mode) && (!php_checkuid(fullpath, NULL, CHECKUID_CHECK_FILE_AND_DIR))) ||
				php_check_open_basedir(fullpath TSRMLS_CC)) {
			efree(fullpath);
			zend_restore_error_handling(&error_handling TSRMLS_CC);
			RETURN_NULL();
		}
	}

	php_sqlite_open(fullpath ? fullpath : filename, (int)mode, NULL, return_value, errmsg, return_value TSRMLS_CC);
	if (fullpath) {
		efree(fullpath);
	}
	zend_restore_error_handling(&error_handling TSRMLS_CC);
}