static ZIPARCHIVE_METHOD(addFile)
{
	struct zip *intern;
	zval *this = getThis();
	char *filename;
	int filename_len;
	char *entry_name = NULL;
	int entry_name_len = 0;
	long offset_start = 0, offset_len = 0;

	if (!this) {
		RETURN_FALSE;
	}

	ZIP_FROM_OBJECT(intern, this);

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "p|sll",
			&filename, &filename_len, &entry_name, &entry_name_len, &offset_start, &offset_len) == FAILURE) {
		return;
	}

	if (filename_len == 0) {
		php_error_docref(NULL TSRMLS_CC, E_NOTICE, "Empty string as filename");
		RETURN_FALSE;
	}

	if (entry_name_len == 0) {
		entry_name = filename;
		entry_name_len = filename_len;
	}

	if (php_zip_add_file(intern, filename, filename_len, 
		entry_name, entry_name_len, 0, 0 TSRMLS_CC) < 0) {
		RETURN_FALSE;
	} else {
		RETURN_TRUE;
	}
}