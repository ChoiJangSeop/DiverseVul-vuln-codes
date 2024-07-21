PHP_FUNCTION(xsl_xsltprocessor_set_parameter)
{
 
	zval *id;
	zval *array_value, **entry, *new_string;
	xsl_object *intern;
	char *string_key, *name, *value, *namespace;
	ulong idx;
	int string_key_len, namespace_len, name_len, value_len;
	DOM_GET_THIS(id);

	if (zend_parse_parameters_ex(ZEND_PARSE_PARAMS_QUIET, ZEND_NUM_ARGS() TSRMLS_CC, "sa", &namespace, &namespace_len, &array_value) == SUCCESS) {
		intern = (xsl_object *)zend_object_store_get_object(id TSRMLS_CC);
		zend_hash_internal_pointer_reset(Z_ARRVAL_P(array_value));

		while (zend_hash_get_current_data(Z_ARRVAL_P(array_value), (void **)&entry) == SUCCESS) {
			SEPARATE_ZVAL(entry);
			convert_to_string_ex(entry);
			
			if (zend_hash_get_current_key_ex(Z_ARRVAL_P(array_value), &string_key, &string_key_len, &idx, 0, NULL) != HASH_KEY_IS_STRING) {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid parameter array");
				RETURN_FALSE;
			}
			
			ALLOC_ZVAL(new_string);
			Z_ADDREF_PP(entry);
			COPY_PZVAL_TO_ZVAL(*new_string, *entry);
			
			zend_hash_update(intern->parameter, string_key, string_key_len, &new_string, sizeof(zval*), NULL);
			zend_hash_move_forward(Z_ARRVAL_P(array_value));
		}
		RETURN_TRUE;

	} else if (zend_parse_parameters_ex(ZEND_PARSE_PARAMS_QUIET, ZEND_NUM_ARGS() TSRMLS_CC, "sss", &namespace, &namespace_len, &name, &name_len, &value, &value_len) == SUCCESS) {
		
		intern = (xsl_object *)zend_object_store_get_object(id TSRMLS_CC);
		
		MAKE_STD_ZVAL(new_string);
		ZVAL_STRING(new_string, value, 1);
		
		zend_hash_update(intern->parameter, name, name_len + 1, &new_string, sizeof(zval*), NULL);
		RETURN_TRUE;
	} else {
		WRONG_PARAM_COUNT;
	}
	
}