PHP_FUNCTION(wddx_deserialize)
{
	zval *packet;
	char *payload;
	int payload_len;
	php_stream *stream = NULL;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &packet) == FAILURE) {
		return;
	}

	if (Z_TYPE_P(packet) == IS_STRING) {
		payload 	= Z_STRVAL_P(packet);
		payload_len = Z_STRLEN_P(packet);
	} else if (Z_TYPE_P(packet) == IS_RESOURCE) {
		php_stream_from_zval(stream, &packet);
		if (stream) {
			payload_len = php_stream_copy_to_mem(stream, &payload, PHP_STREAM_COPY_ALL, 0);
		}
	} else {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Expecting parameter 1 to be a string or a stream");
		return;
	}

	if (payload_len == 0) {
		return;
	}

	php_wddx_deserialize_ex(payload, payload_len, return_value);
		
	if (stream) {
		pefree(payload, 0);
	}
}