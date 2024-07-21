XMLRPC_VALUE_TYPE get_zval_xmlrpc_type(zval* value, zval* newvalue) /* {{{ */
{
	XMLRPC_VALUE_TYPE type = xmlrpc_none;

	if (value) {
		switch (Z_TYPE_P(value)) {
			case IS_NULL:
				type = xmlrpc_base64;
				break;
#ifndef BOOL_AS_LONG

			/* Right thing to do, but it breaks some legacy code. */
			case IS_TRUE:
			case IS_FALSE:
				type = xmlrpc_boolean;
				break;
#else
			case IS_BOOL:
#endif
			case IS_LONG:
			case IS_RESOURCE:
				type = xmlrpc_int;
				break;
			case IS_DOUBLE:
				type = xmlrpc_double;
				break;
			case IS_CONSTANT:
				type = xmlrpc_string;
				break;
			case IS_STRING:
				type = xmlrpc_string;
				break;
			case IS_ARRAY:
				type = xmlrpc_vector;
				break;
			case IS_OBJECT:
				{
					zval* attr;
					type = xmlrpc_vector;

					if ((attr = zend_hash_str_find(Z_OBJPROP_P(value), OBJECT_TYPE_ATTR, sizeof(OBJECT_TYPE_ATTR) - 1)) != NULL) {
						if (Z_TYPE_P(attr) == IS_STRING) {
							type = xmlrpc_str_as_type(Z_STRVAL_P(attr));
						}
					}
					break;
				}
		}

		/* if requested, return an unmolested (magic removed) copy of the value */
		if (newvalue) {
			zval* val;

			if ((type == xmlrpc_base64 && Z_TYPE_P(value) != IS_NULL) || type == xmlrpc_datetime) {
				if ((val = zend_hash_str_find(Z_OBJPROP_P(value), OBJECT_VALUE_ATTR, sizeof(OBJECT_VALUE_ATTR) - 1)) != NULL) {
					ZVAL_COPY_VALUE(newvalue, val);
				}
			} else {
				ZVAL_COPY_VALUE(newvalue, value);
			}
		}
	}

	return type;
}