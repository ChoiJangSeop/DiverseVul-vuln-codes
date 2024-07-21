static zend_object_value php_snmp_object_new(zend_class_entry *class_type TSRMLS_DC) /* {{{ */
{
	zend_object_value retval;
	php_snmp_object *intern;

	/* Allocate memory for it */
	intern = emalloc(sizeof(php_snmp_object));
	memset(&intern->zo, 0, sizeof(php_snmp_object));

	zend_object_std_init(&intern->zo, class_type TSRMLS_CC);
	object_properties_init(&intern->zo, class_type);

	retval.handle = zend_objects_store_put(intern, (zend_objects_store_dtor_t)zend_objects_destroy_object, (zend_objects_free_object_storage_t) php_snmp_object_free_storage, NULL TSRMLS_CC);
	retval.handlers = (zend_object_handlers *) &php_snmp_object_handlers;

	return retval;
	
}