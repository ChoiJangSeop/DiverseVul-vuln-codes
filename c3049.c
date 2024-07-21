PHP_FUNCTION(xsl_xsltprocessor_register_php_functions)
{
	zval *id;
	xsl_object *intern;
	zval *array_value, **entry, *new_string;
	int  name_len = 0;
	char *name;

	DOM_GET_THIS(id);
	
	if (zend_parse_parameters_ex(ZEND_PARSE_PARAMS_QUIET, ZEND_NUM_ARGS() TSRMLS_CC, "a",  &array_value) == SUCCESS) {
		intern = (xsl_object *)zend_object_store_get_object(id TSRMLS_CC);
		zend_hash_internal_pointer_reset(Z_ARRVAL_P(array_value));

		while (zend_hash_get_current_data(Z_ARRVAL_P(array_value), (void **)&entry) == SUCCESS) {
			SEPARATE_ZVAL(entry);
			convert_to_string_ex(entry);
			
			MAKE_STD_ZVAL(new_string);
			ZVAL_LONG(new_string,1);
		
			zend_hash_update(intern->registered_phpfunctions, Z_STRVAL_PP(entry), Z_STRLEN_PP(entry) + 1, &new_string, sizeof(zval*), NULL);
			zend_hash_move_forward(Z_ARRVAL_P(array_value));
		}
		intern->registerPhpFunctions = 2;

	} else if (zend_parse_parameters_ex(ZEND_PARSE_PARAMS_QUIET, ZEND_NUM_ARGS() TSRMLS_CC, "s",  &name, &name_len) == SUCCESS) {
		intern = (xsl_object *)zend_object_store_get_object(id TSRMLS_CC);
		
		MAKE_STD_ZVAL(new_string);
		ZVAL_LONG(new_string,1);
		zend_hash_update(intern->registered_phpfunctions, name, name_len + 1, &new_string, sizeof(zval*), NULL);
		intern->registerPhpFunctions = 2;
		
	} else {
		intern = (xsl_object *)zend_object_store_get_object(id TSRMLS_CC);
		intern->registerPhpFunctions = 1;
	}
	
}