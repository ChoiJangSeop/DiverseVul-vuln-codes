PHPAPI void php_register_variable_safe(char *var, char *strval, int str_len, zval *track_vars_array TSRMLS_DC)
{
	zval new_entry;
	assert(strval != NULL);
	
	/* Prepare value */
	Z_STRLEN(new_entry) = str_len;
	Z_STRVAL(new_entry) = estrndup(strval, Z_STRLEN(new_entry));
	Z_TYPE(new_entry) = IS_STRING;

	php_register_variable_ex(var, &new_entry, track_vars_array TSRMLS_CC);
}