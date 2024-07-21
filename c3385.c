PHP_FUNCTION(wddx_serialize_vars)
{
	int num_args, i;
	wddx_packet *packet;
	zval ***args = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "+", &args, &num_args) == FAILURE) {
		return;
	}
		
	packet = php_wddx_constructor();

	php_wddx_packet_start(packet, NULL, 0);
	php_wddx_add_chunk_static(packet, WDDX_STRUCT_S);
	
	for (i=0; i<num_args; i++) {
		if (Z_TYPE_PP(args[i]) != IS_ARRAY && Z_TYPE_PP(args[i]) != IS_OBJECT) {
			convert_to_string_ex(args[i]);
		}
		php_wddx_add_var(packet, *args[i]);
	}	
	
	php_wddx_add_chunk_static(packet, WDDX_STRUCT_E);
	php_wddx_packet_end(packet);

	efree(args);

	ZVAL_STRINGL(return_value, packet->c, packet->len, 1);
	smart_str_free(packet);
	efree(packet);
}