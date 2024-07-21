static char **php_xsl_xslt_make_params(HashTable *parht, int xpath_params TSRMLS_DC)
{
	
	int parsize;
	zval **value;
	char *xpath_expr, *string_key = NULL;
	ulong num_key;
	char **params = NULL;
	int i = 0;

	parsize = (2 * zend_hash_num_elements(parht) + 1) * sizeof(char *);
	params = (char **)safe_emalloc((2 * zend_hash_num_elements(parht) + 1), sizeof(char *), 0);
	memset((char *)params, 0, parsize);

	for (zend_hash_internal_pointer_reset(parht);
		zend_hash_get_current_data(parht, (void **)&value) == SUCCESS;
		zend_hash_move_forward(parht)) {

		if (zend_hash_get_current_key(parht, &string_key, &num_key, 1) != HASH_KEY_IS_STRING) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid argument or parameter array");
			efree(params);
			return NULL;
		} else {
			if (Z_TYPE_PP(value) != IS_STRING) {
				SEPARATE_ZVAL(value);
				convert_to_string(*value);
			}
			
			if (!xpath_params) {
				xpath_expr = php_xsl_xslt_string_to_xpathexpr(Z_STRVAL_PP(value) TSRMLS_CC);
			} else {
				xpath_expr = estrndup(Z_STRVAL_PP(value), Z_STRLEN_PP(value));
			}
			if (xpath_expr) {
				params[i++] = string_key;
				params[i++] = xpath_expr;
			} else {
				efree(string_key);
			}
		}
	}

	params[i++] = NULL;

	return params;
}