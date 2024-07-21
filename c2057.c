static ZIPARCHIVE_METHOD(extractTo)
{
	struct zip *intern;

	zval *this = getThis();
	zval *zval_files = NULL;
	zval **zval_file = NULL;
	php_stream_statbuf ssb;
	char *pathto;
	int pathto_len;
	int ret, i;

	int nelems;

	if (!this) {
		RETURN_FALSE;
	}

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|z", &pathto, &pathto_len, &zval_files) == FAILURE) {
		return;
	}

	if (pathto_len < 1) {
		RETURN_FALSE;
	}

	if (php_stream_stat_path(pathto, &ssb) < 0) {
		ret = php_stream_mkdir(pathto, 0777,  PHP_STREAM_MKDIR_RECURSIVE, NULL);
		if (!ret) {
			RETURN_FALSE;
		}
	}

	ZIP_FROM_OBJECT(intern, this);
	if (zval_files && (Z_TYPE_P(zval_files) != IS_NULL)) {
		switch (Z_TYPE_P(zval_files)) {
			case IS_STRING:
				if (!php_zip_extract_file(intern, pathto, Z_STRVAL_P(zval_files), Z_STRLEN_P(zval_files) TSRMLS_CC)) {
					RETURN_FALSE;
				}
				break;
			case IS_ARRAY:
				nelems = zend_hash_num_elements(Z_ARRVAL_P(zval_files));
				if (nelems == 0 ) {
					RETURN_FALSE;
				}
				for (i = 0; i < nelems; i++) {
					if (zend_hash_index_find(Z_ARRVAL_P(zval_files), i, (void **) &zval_file) == SUCCESS) {
						switch (Z_TYPE_PP(zval_file)) {
							case IS_LONG:
								break;
							case IS_STRING:
								if (!php_zip_extract_file(intern, pathto, Z_STRVAL_PP(zval_file), Z_STRLEN_PP(zval_file) TSRMLS_CC)) {
									RETURN_FALSE;
								}
								break;
						}
					}
				}
				break;
			case IS_LONG:
			default:
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid argument, expect string or array of strings");
				break;
		}
	} else {
        /* Extract all files */
        int filecount = zip_get_num_files(intern);

        if (filecount == -1) {
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "Illegal archive");
            RETURN_FALSE;
        }

        for (i = 0; i < filecount; i++) {
			char *file = (char*)zip_get_name(intern, i, ZIP_FL_UNCHANGED);
            if (!php_zip_extract_file(intern, pathto, file, strlen(file) TSRMLS_CC)) {
                RETURN_FALSE;
            }
        }
    }
	RETURN_TRUE;
}