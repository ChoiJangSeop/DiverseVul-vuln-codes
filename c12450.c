static PHP_NAMED_FUNCTION(zif_zip_entry_read)
{
	zval * zip_entry;
	zend_long len = 0;
	zip_read_rsrc * zr_rsrc;
	zend_string *buffer;
	int n = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "r|l", &zip_entry, &len) == FAILURE) {
		return;
	}

	if ((zr_rsrc = (zip_read_rsrc *)zend_fetch_resource(Z_RES_P(zip_entry), le_zip_entry_name, le_zip_entry)) == NULL) {
		RETURN_FALSE;
	}

	if (len <= 0) {
		len = 1024;
	}

	if (zr_rsrc->zf) {
		buffer = zend_string_alloc(len, 0);
		n = zip_fread(zr_rsrc->zf, ZSTR_VAL(buffer), ZSTR_LEN(buffer));
		if (n > 0) {
			ZSTR_VAL(buffer)[n] = '\0';
			ZSTR_LEN(buffer) = n;
			RETURN_NEW_STR(buffer);
		} else {
			zend_string_free(buffer);
			RETURN_EMPTY_STRING()
		}
	} else {
		RETURN_FALSE;
	}
}