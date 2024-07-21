PHP_FUNCTION(wddx_packet_end)
{
	zval *packet_id;
	wddx_packet *packet = NULL;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &packet_id) == FAILURE) {
		return;
	}

	ZEND_FETCH_RESOURCE(packet, wddx_packet *, &packet_id, -1, "WDDX packet ID", le_wddx);
			
	php_wddx_add_chunk_static(packet, WDDX_STRUCT_E);	
	
	php_wddx_packet_end(packet);

	ZVAL_STRINGL(return_value, packet->c, packet->len, 1);

	zend_list_delete(Z_LVAL_P(packet_id));
}