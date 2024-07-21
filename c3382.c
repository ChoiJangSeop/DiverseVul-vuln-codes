PHP_FUNCTION(wddx_add_vars)
{
	int num_args, i;
	zval ***args = NULL;
	zval *packet_id;
	wddx_packet *packet = NULL;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r+", &packet_id, &args, &num_args) == FAILURE) {
		return;
	}

	if (!ZEND_FETCH_RESOURCE_NO_RETURN(packet, wddx_packet *, &packet_id, -1, "WDDX packet ID", le_wddx)) {
		efree(args);
		RETURN_FALSE;
	}
	
	if (!packet) {
		efree(args);
		RETURN_FALSE;
	}
		
	for (i=0; i<num_args; i++) {
		if (Z_TYPE_PP(args[i]) != IS_ARRAY && Z_TYPE_PP(args[i]) != IS_OBJECT) {
			convert_to_string_ex(args[i]);
		}
		php_wddx_add_var(packet, (*args[i]));
	}

	efree(args);
	RETURN_TRUE;
}