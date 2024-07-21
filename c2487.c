
static int php_date_timezone_initialize_from_hash(zval **return_value, php_timezone_obj **tzobj, HashTable *myht TSRMLS_DC)
{
	zval            **z_timezone = NULL;
	zval            **z_timezone_type = NULL;

	if (zend_hash_find(myht, "timezone_type", 14, (void**) &z_timezone_type) == SUCCESS) {
		if (zend_hash_find(myht, "timezone", 9, (void**) &z_timezone) == SUCCESS) {
			convert_to_long(*z_timezone_type);
			if (SUCCESS == timezone_initialize(*tzobj, Z_STRVAL_PP(z_timezone) TSRMLS_CC)) {
				return SUCCESS;
			}
		}
	}
	return FAILURE;