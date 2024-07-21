static PHP_FUNCTION(libxml_disable_entity_loader)
{
	zend_bool disable = 1;
	xmlParserInputBufferCreateFilenameFunc old;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|b", &disable) == FAILURE) {
		return;
	}

	if (disable == 0) {
		old = xmlParserInputBufferCreateFilenameDefault(php_libxml_input_buffer_create_filename);
	} else {
		old = xmlParserInputBufferCreateFilenameDefault(php_libxml_input_buffer_noload);
	}

	if (old == php_libxml_input_buffer_noload) {
		RETURN_TRUE;
	}

	RETURN_FALSE;
}