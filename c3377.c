PHP_FUNCTION(wddx_serialize_value)
{
	zval *var;
	char *comment = NULL;
	int comment_len = 0;
	wddx_packet *packet;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z|s", &var, &comment, &comment_len) == FAILURE) {
		return;
	}
	
	packet = php_wddx_constructor();

	php_wddx_packet_start(packet, comment, comment_len);
	php_wddx_serialize_var(packet, var, NULL, 0 TSRMLS_CC);
	php_wddx_packet_end(packet);
					
	ZVAL_STRINGL(return_value, packet->c, packet->len, 1);
	smart_str_free(packet);
	efree(packet);
}