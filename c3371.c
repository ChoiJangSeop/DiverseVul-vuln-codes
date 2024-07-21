PHP_FUNCTION(wddx_packet_start)
{
	char *comment = NULL;
	int comment_len = 0;
	wddx_packet *packet;

	comment = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|s", &comment, &comment_len) == FAILURE) {
		return;
	}

	packet = php_wddx_constructor();
	
	php_wddx_packet_start(packet, comment, comment_len);
	php_wddx_add_chunk_static(packet, WDDX_STRUCT_S);

	ZEND_REGISTER_RESOURCE(return_value, packet, le_wddx);
}