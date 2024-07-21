static void php_zip_add_from_pattern(INTERNAL_FUNCTION_PARAMETERS, int type) /* {{{ */
{
	struct zip *intern;
	zval *this = getThis();
	char *pattern;
	char *path = NULL;
	char *remove_path = NULL;
	char *add_path = NULL;
	int pattern_len, add_path_len = 0, remove_path_len = 0, path_len = 0;
	long remove_all_path = 0;
	long flags = 0;
	zval *options = NULL;
	int found;

	if (!this) {
		RETURN_FALSE;
	}

	ZIP_FROM_OBJECT(intern, this);
	/* 1 == glob, 2==pcre */
	if (type == 1) {
		if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "p|la", 
					&pattern, &pattern_len, &flags, &options) == FAILURE) {
			return;
		}
	} else {
		if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "p|sa", 
					&pattern, &pattern_len, &path, &path_len, &options) == FAILURE) {
			return;
		}
	}

	if (pattern_len == 0) {
		php_error_docref(NULL TSRMLS_CC, E_NOTICE, "Empty string as pattern");
		RETURN_FALSE;
	}
	if (options && (php_zip_parse_options(options, &remove_all_path, &remove_path, &remove_path_len,
			&add_path, &add_path_len TSRMLS_CC) < 0)) {
		RETURN_FALSE;
	}

	if (remove_path && remove_path_len > 1 && (remove_path[strlen(remove_path) - 1] == '/' ||
		remove_path[strlen(remove_path) - 1] == '\\')) {
		remove_path[strlen(remove_path) - 1] = '\0';
	}

	if (type == 1) {
		found = php_zip_glob(pattern, pattern_len, flags, return_value TSRMLS_CC);
	} else {
		found = php_zip_pcre(pattern, pattern_len, path, path_len, return_value TSRMLS_CC);
	}

	if (found > 0) {
		int i;
		zval **zval_file = NULL;

		for (i = 0; i < found; i++) {
			char *file, *file_stripped, *entry_name;
			size_t entry_name_len, file_stripped_len;
			char entry_name_buf[MAXPATHLEN];
			char *basename = NULL;

			if (zend_hash_index_find(Z_ARRVAL_P(return_value), i, (void **) &zval_file) == SUCCESS) {
				file = Z_STRVAL_PP(zval_file);
				if (remove_all_path) {
					php_basename(Z_STRVAL_PP(zval_file), Z_STRLEN_PP(zval_file), NULL, 0,
									&basename, (size_t *)&file_stripped_len TSRMLS_CC);
					file_stripped = basename;
				} else if (remove_path && strstr(Z_STRVAL_PP(zval_file), remove_path) != NULL) {
					file_stripped = Z_STRVAL_PP(zval_file) + remove_path_len + 1;
					file_stripped_len = Z_STRLEN_PP(zval_file) - remove_path_len - 1;
				} else {
					file_stripped = Z_STRVAL_PP(zval_file);
					file_stripped_len = Z_STRLEN_PP(zval_file);
				}

				if (add_path) {
					if ((add_path_len + file_stripped_len) > MAXPATHLEN) {
						php_error_docref(NULL TSRMLS_CC, E_WARNING, "Entry name too long (max: %d, %ld given)", 
						MAXPATHLEN - 1, (add_path_len + file_stripped_len));
						zval_dtor(return_value);
						RETURN_FALSE;
					}

					snprintf(entry_name_buf, MAXPATHLEN, "%s%s", add_path, file_stripped);
					entry_name = entry_name_buf; 
					entry_name_len = strlen(entry_name);
				} else {
					entry_name = Z_STRVAL_PP(zval_file);
					entry_name_len = Z_STRLEN_PP(zval_file);
				}
				if (basename) {
					efree(basename);
					basename = NULL;
				}
				if (php_zip_add_file(intern, Z_STRVAL_PP(zval_file), Z_STRLEN_PP(zval_file), 
					entry_name, entry_name_len, 0, 0 TSRMLS_CC) < 0) {
					zval_dtor(return_value);
					RETURN_FALSE;
				}
			}
		}
	}
}