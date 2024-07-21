static void php_wddx_serialize_number(wddx_packet *packet, zval *var)
{
	char tmp_buf[WDDX_BUF_LEN];
	zval tmp;
	
	tmp = *var;
	zval_copy_ctor(&tmp);
	convert_to_string(&tmp);
	snprintf(tmp_buf, sizeof(tmp_buf), WDDX_NUMBER, Z_STRVAL(tmp));
	zval_dtor(&tmp);

	php_wddx_add_chunk(packet, tmp_buf);	
}