PHP_MSHUTDOWN_FUNCTION(snmp)
{
	snmp_shutdown("snmpapp");
	
	zend_hash_destroy(&php_snmp_properties);

	return SUCCESS;
}