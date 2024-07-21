void php_wddx_serialize_var(wddx_packet *packet, zval *var, char *name, int name_len TSRMLS_DC)
{
	HashTable *ht;

	if (name) {
		size_t name_esc_len;
		char *tmp_buf, *name_esc;

		name_esc = php_escape_html_entities(name, name_len, &name_esc_len, 0, ENT_QUOTES, NULL TSRMLS_CC);
		tmp_buf = emalloc(name_esc_len + sizeof(WDDX_VAR_S));
		snprintf(tmp_buf, name_esc_len + sizeof(WDDX_VAR_S), WDDX_VAR_S, name_esc);
		php_wddx_add_chunk(packet, tmp_buf);
		efree(tmp_buf);
		efree(name_esc);
	}
	
	switch(Z_TYPE_P(var)) {
		case IS_STRING:
			php_wddx_serialize_string(packet, var TSRMLS_CC);
			break;
			
		case IS_LONG:
		case IS_DOUBLE:
			php_wddx_serialize_number(packet, var);
			break;

		case IS_BOOL:
			php_wddx_serialize_boolean(packet, var);
			break;

		case IS_NULL:
			php_wddx_serialize_unset(packet);
			break;
		
		case IS_ARRAY:
			ht = Z_ARRVAL_P(var);
			if (ht->nApplyCount > 1) {
				php_error_docref(NULL TSRMLS_CC, E_RECOVERABLE_ERROR, "WDDX doesn't support circular references");
				return;
			}
			ht->nApplyCount++;															
			php_wddx_serialize_array(packet, var);
			ht->nApplyCount--;
			break;

		case IS_OBJECT:
			ht = Z_OBJPROP_P(var);
			if (ht->nApplyCount > 1) {
				php_error_docref(NULL TSRMLS_CC, E_RECOVERABLE_ERROR, "WDDX doesn't support circular references");
				return;
			}
			ht->nApplyCount++;
 			php_wddx_serialize_object(packet, var);
			ht->nApplyCount--;
			break;
	}
	
	if (name) {
		php_wddx_add_chunk_static(packet, WDDX_VAR_E);
	}
}