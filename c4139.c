static int php_snmp_write_valueretrieval(php_snmp_object *snmp_object, zval *newval TSRMLS_DC)
{
	zval ztmp;
	int ret = SUCCESS;

	if (Z_TYPE_P(newval) != IS_LONG) {
		ztmp = *newval;
		zval_copy_ctor(&ztmp);
		convert_to_long(&ztmp);
		newval = &ztmp;
	}

	if (Z_LVAL_P(newval) >= 0 && Z_LVAL_P(newval) <= (SNMP_VALUE_LIBRARY|SNMP_VALUE_PLAIN|SNMP_VALUE_OBJECT)) {
		snmp_object->valueretrieval = Z_LVAL_P(newval);
	} else {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unknown SNMP value retrieval method '%ld'", Z_LVAL_P(newval));
		ret = FAILURE;
	}
	
	if (newval == &ztmp) {
		zval_dtor(newval);
	}

	return ret;
}