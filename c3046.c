PHP_FUNCTION(xsl_xsltprocessor_get_parameter)
{
	zval *id;
	int name_len = 0, namespace_len = 0;
	char *name, *namespace;
	zval **value;
	xsl_object *intern;

	DOM_GET_THIS(id);
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss", &namespace, &namespace_len, &name, &name_len) == FAILURE) {
		RETURN_FALSE;
	}
	intern = (xsl_object *)zend_object_store_get_object(id TSRMLS_CC);
	if ( zend_hash_find(intern->parameter, name, name_len + 1,  (void**) &value) == SUCCESS) {
		convert_to_string_ex(value);
		RETVAL_STRING(Z_STRVAL_PP(value),1);
	} else {
		RETURN_FALSE;
	}
}