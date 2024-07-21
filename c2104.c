PHP_FUNCTION(oci_lob_export)
{	
	zval **tmp, *z_descriptor = getThis();
	php_oci_descriptor *descriptor;
	char *filename;
	char *buffer;
	int filename_len;
	long start = -1, length = -1, block_length;
	php_stream *stream;
	ub4 lob_length;

	if (getThis()) {
		if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|ll", &filename, &filename_len, &start, &length) == FAILURE) {
			return;
		}
	
		if (ZEND_NUM_ARGS() > 1 && start < 0) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Start parameter must be greater than or equal to 0");
			RETURN_FALSE;
		}
		if (ZEND_NUM_ARGS() > 2 && length < 0) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Length parameter must be greater than or equal to 0");
			RETURN_FALSE;
		}
	}
	else {
		if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Os|ll", &z_descriptor, oci_lob_class_entry_ptr, &filename, &filename_len, &start, &length) == FAILURE) {
			return;
		}
			
		if (ZEND_NUM_ARGS() > 2 && start < 0) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Start parameter must be greater than or equal to 0");
			RETURN_FALSE;
		}
		if (ZEND_NUM_ARGS() > 3 && length < 0) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Length parameter must be greater than or equal to 0");
			RETURN_FALSE;
		}
	}

	if (strlen(filename) != filename_len) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Filename cannot contain null bytes");
		RETURN_FALSE;  
	}

	if (zend_hash_find(Z_OBJPROP_P(z_descriptor), "descriptor", sizeof("descriptor"), (void **)&tmp) == FAILURE) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to find descriptor property");
		RETURN_FALSE;
	}
	
	PHP_OCI_ZVAL_TO_DESCRIPTOR(*tmp, descriptor);
	
	if (php_oci_lob_get_length(descriptor, &lob_length TSRMLS_CC)) {
		RETURN_FALSE;
	}		
	
	if (start == -1) {
		start = 0;
	}

	if (length == -1) {
		length = lob_length - descriptor->lob_current_position;
	}
	
	if (length == 0) {
		/* nothing to write, fail silently */
		RETURN_FALSE;
	}
	
	if (PG(safe_mode) && (!php_checkuid(filename, NULL, CHECKUID_CHECK_FILE_AND_DIR))) {
		RETURN_FALSE;
	}

	if (php_check_open_basedir(filename TSRMLS_CC)) {
		RETURN_FALSE;
	}

	stream = php_stream_open_wrapper_ex(filename, "w", ENFORCE_SAFE_MODE | REPORT_ERRORS, NULL, NULL);

	block_length = PHP_OCI_LOB_BUFFER_SIZE;
	if (block_length > length) {
		block_length = length;
	}

	while(length > 0) {
		ub4 tmp_bytes_read = 0;
		if (php_oci_lob_read(descriptor, block_length, start, &buffer, &tmp_bytes_read TSRMLS_CC)) {
			php_stream_close(stream);
			RETURN_FALSE;
		}
		if (tmp_bytes_read && !php_stream_write(stream, buffer, tmp_bytes_read)) {
			php_stream_close(stream);
			efree(buffer);
			RETURN_FALSE;
		}
		if (buffer) {
			efree(buffer);
		}
		
		length -= tmp_bytes_read;
		descriptor->lob_current_position += tmp_bytes_read;
		start += tmp_bytes_read;

		if (block_length > length) {
			block_length = length;
		}
	}

	php_stream_close(stream);
	RETURN_TRUE;
}