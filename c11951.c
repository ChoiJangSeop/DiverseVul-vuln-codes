PHP_FUNCTION(oci_lob_load)
{
	zval **tmp, *z_descriptor = getThis();
	php_oci_descriptor *descriptor;
	char *buffer = NULL;
	ub4 buffer_len;

	if (!getThis()) {
		if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O", &z_descriptor, oci_lob_class_entry_ptr) == FAILURE) {
			return;
		}	
	}

	if (strlen(filename) != filename_len) {
		RETURN_FALSE;
	}
	
	if (zend_hash_find(Z_OBJPROP_P(z_descriptor), "descriptor", sizeof("descriptor"), (void **)&tmp) == FAILURE) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to find descriptor property");
		RETURN_FALSE;
	}
	
	PHP_OCI_ZVAL_TO_DESCRIPTOR(*tmp, descriptor);

	if (php_oci_lob_read(descriptor, -1, 0, &buffer, &buffer_len TSRMLS_CC)) {
		RETURN_FALSE;
	}
	if (buffer_len > 0) {
		RETURN_STRINGL(buffer, buffer_len, 0);
	}
	else {
		RETURN_EMPTY_STRING();
	}
}