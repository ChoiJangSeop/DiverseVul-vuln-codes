static int php_zip_parse_options(zval *options, long *remove_all_path, 
	char **remove_path, int *remove_path_len, char **add_path, int *add_path_len TSRMLS_DC) /* {{{ */
{
	zval **option;
	if (zend_hash_find(HASH_OF(options), "remove_all_path", sizeof("remove_all_path"), (void **)&option) == SUCCESS) {
		long opt;
		if (Z_TYPE_PP(option) != IS_LONG) {
			zval tmp = **option;
			zval_copy_ctor(&tmp);
			convert_to_long(&tmp);
			opt = Z_LVAL(tmp);
		} else {
			opt = Z_LVAL_PP(option);
		}
		*remove_all_path = opt;
	}

	/* If I add more options, it would make sense to create a nice static struct and loop over it. */
	if (zend_hash_find(HASH_OF(options), "remove_path", sizeof("remove_path"), (void **)&option) == SUCCESS) {
		if (Z_TYPE_PP(option) != IS_STRING) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "remove_path option expected to be a string");
			return -1;
		}

		if (Z_STRLEN_PP(option) < 1) {
			php_error_docref(NULL TSRMLS_CC, E_NOTICE, "Empty string given as remove_path option");
			return -1;
		}

		if (Z_STRLEN_PP(option) >= MAXPATHLEN) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "remove_path string is too long (max: %i, %i given)", 
						MAXPATHLEN - 1, Z_STRLEN_PP(option));
			return -1;
		}
		*remove_path_len = Z_STRLEN_PP(option); 
		*remove_path = Z_STRVAL_PP(option);
	}

	if (zend_hash_find(HASH_OF(options), "add_path", sizeof("add_path"), (void **)&option) == SUCCESS) {
		if (Z_TYPE_PP(option) != IS_STRING) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "add_path option expected to be a string");
			return -1;
		}

		if (Z_STRLEN_PP(option) < 1) {
			php_error_docref(NULL TSRMLS_CC, E_NOTICE, "Empty string given as the add_path option");
			return -1;
		}

		if (Z_STRLEN_PP(option) >= MAXPATHLEN) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "add_path string too long (max: %i, %i given)", 
						MAXPATHLEN - 1, Z_STRLEN_PP(option));
			return -1;
		}
		*add_path_len = Z_STRLEN_PP(option); 
		*add_path = Z_STRVAL_PP(option);
	}
	return 1;
}