static int php_snmp_read_info(php_snmp_object *snmp_object, zval **retval TSRMLS_DC)
{
	zval *val;

	MAKE_STD_ZVAL(*retval);
	array_init(*retval);

	if (snmp_object->session == NULL) {
		return SUCCESS;
	}
		
	MAKE_STD_ZVAL(val);
	ZVAL_STRINGL(val, snmp_object->session->peername, strlen(snmp_object->session->peername), 1);
	add_assoc_zval(*retval, "hostname", val);
	
	MAKE_STD_ZVAL(val);
	ZVAL_LONG(val, snmp_object->session->remote_port);
	add_assoc_zval(*retval, "port", val);
	
	MAKE_STD_ZVAL(val);
	ZVAL_LONG(val, snmp_object->session->timeout);
	add_assoc_zval(*retval, "timeout", val);
	
	MAKE_STD_ZVAL(val);
	ZVAL_LONG(val, snmp_object->session->retries);
	add_assoc_zval(*retval, "retries", val);
	
	return SUCCESS;
}