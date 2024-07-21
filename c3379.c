PS_SERIALIZER_ENCODE_FUNC(wddx)
{
	wddx_packet *packet;
	PS_ENCODE_VARS;

	packet = php_wddx_constructor();

	php_wddx_packet_start(packet, NULL, 0);
	php_wddx_add_chunk_static(packet, WDDX_STRUCT_S);
	
	PS_ENCODE_LOOP(
		php_wddx_serialize_var(packet, *struc, key, key_length TSRMLS_CC);
	);
	
	php_wddx_add_chunk_static(packet, WDDX_STRUCT_E);
	php_wddx_packet_end(packet);
	*newstr = php_wddx_gather(packet);
	php_wddx_destructor(packet);
	
	if (newlen) {
		*newlen = strlen(*newstr);
	}

	return SUCCESS;
}