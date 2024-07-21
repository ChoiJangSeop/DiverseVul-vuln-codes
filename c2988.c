PHPAPI void php_var_export_ex(zval **struc, int level, smart_str *buf TSRMLS_DC) /* {{{ */
{
	HashTable *myht;
	char *tmp_str, *tmp_str2;
	int tmp_len, tmp_len2;
	const char *class_name;
	zend_uint class_name_len;

	switch (Z_TYPE_PP(struc)) {
	case IS_BOOL:
		if (Z_LVAL_PP(struc)) {
			smart_str_appendl(buf, "true", 4);
		} else {
			smart_str_appendl(buf, "false", 5);
		}
		break;
	case IS_NULL:
		smart_str_appendl(buf, "NULL", 4);
		break;
	case IS_LONG:
		smart_str_append_long(buf, Z_LVAL_PP(struc));
		break;
	case IS_DOUBLE:
		tmp_len = spprintf(&tmp_str, 0,"%.*H", PG(serialize_precision), Z_DVAL_PP(struc));
		smart_str_appendl(buf, tmp_str, tmp_len);
		efree(tmp_str);
		break;
	case IS_STRING:
		tmp_str = php_addcslashes(Z_STRVAL_PP(struc), Z_STRLEN_PP(struc), &tmp_len, 0, "'\\", 2 TSRMLS_CC);
		tmp_str2 = php_str_to_str_ex(tmp_str, tmp_len, "\0", 1, "' . \"\\0\" . '", 12, &tmp_len2, 0, NULL);

		smart_str_appendc(buf, '\'');
		smart_str_appendl(buf, tmp_str2, tmp_len2);
		smart_str_appendc(buf, '\'');

		efree(tmp_str2);
		efree(tmp_str);
		break;
	case IS_ARRAY:
		myht = Z_ARRVAL_PP(struc);
		if(myht && myht->nApplyCount > 0){
			smart_str_appendl(buf, "NULL", 4);
			zend_error(E_WARNING, "var_export does not handle circular references");
			return;
		}
		if (level > 1) {
			smart_str_appendc(buf, '\n');
			buffer_append_spaces(buf, level - 1);
		}
		smart_str_appendl(buf, "array (\n", 8);
		zend_hash_apply_with_arguments(myht TSRMLS_CC, (apply_func_args_t) php_array_element_export, 2, level, buf);

		if (level > 1) {
			buffer_append_spaces(buf, level - 1);
		}
		smart_str_appendc(buf, ')');
    
		break;

	case IS_OBJECT:
		myht = Z_OBJPROP_PP(struc);
		if(myht && myht->nApplyCount > 0){
			smart_str_appendl(buf, "NULL", 4);
			zend_error(E_WARNING, "var_export does not handle circular references");
			return;
		}
		if (level > 1) {
			smart_str_appendc(buf, '\n');
			buffer_append_spaces(buf, level - 1);
		}
		Z_OBJ_HANDLER(**struc, get_class_name)(*struc, &class_name, &class_name_len, 0 TSRMLS_CC);

		smart_str_appendl(buf, class_name, class_name_len);
		smart_str_appendl(buf, "::__set_state(array(\n", 21);

		efree((char*)class_name);
		if (myht) {
			zend_hash_apply_with_arguments(myht TSRMLS_CC, (apply_func_args_t) php_object_element_export, 1, level, buf);
		}
		if (level > 1) {
			buffer_append_spaces(buf, level - 1);
		}
		smart_str_appendl(buf, "))", 2);

		break;
	default:
		smart_str_appendl(buf, "NULL", 4);
		break;
	}
}