static int php_snmp_write_oid_output_format(php_snmp_object *snmp_object, zval *newval TSRMLS_DC)
{
	zval ztmp;
	int ret = SUCCESS;
	if (Z_TYPE_P(newval) != IS_LONG) {
		ztmp = *newval;
		zval_copy_ctor(&ztmp);
		convert_to_long(&ztmp);
		newval = &ztmp;
	}
	
	switch(Z_LVAL_P(newval)) {
		case NETSNMP_OID_OUTPUT_SUFFIX:
		case NETSNMP_OID_OUTPUT_MODULE:
		case NETSNMP_OID_OUTPUT_FULL:
		case NETSNMP_OID_OUTPUT_NUMERIC:
		case NETSNMP_OID_OUTPUT_UCD:
		case NETSNMP_OID_OUTPUT_NONE:
			snmp_object->oid_output_format = Z_LVAL_P(newval);
			break;
		default:
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unknown SNMP output print format '%ld'", Z_LVAL_P(newval));
			ret = FAILURE;
			break;
	}

	if (newval == &ztmp) {
		zval_dtor(newval);
	}
	return ret;
}