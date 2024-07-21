static void php_wddx_add_var(wddx_packet *packet, zval *name_var)
{
	zval **val;
	HashTable *target_hash;
	TSRMLS_FETCH();

	if (Z_TYPE_P(name_var) == IS_STRING) {
		if (!EG(active_symbol_table)) {
			zend_rebuild_symbol_table(TSRMLS_C);
		}
		if (zend_hash_find(EG(active_symbol_table), Z_STRVAL_P(name_var),
							Z_STRLEN_P(name_var)+1, (void**)&val) != FAILURE) {
			php_wddx_serialize_var(packet, *val, Z_STRVAL_P(name_var), Z_STRLEN_P(name_var) TSRMLS_CC);
		}		
	} else if (Z_TYPE_P(name_var) == IS_ARRAY || Z_TYPE_P(name_var) == IS_OBJECT)	{
		int is_array = Z_TYPE_P(name_var) == IS_ARRAY;
		
		target_hash = HASH_OF(name_var);
		
		if (is_array && target_hash->nApplyCount > 1) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "recursion detected");
			return;
		}

		zend_hash_internal_pointer_reset(target_hash);

		while(zend_hash_get_current_data(target_hash, (void**)&val) == SUCCESS) {
			if (is_array) {
				target_hash->nApplyCount++;
			}

			php_wddx_add_var(packet, *val);

			if (is_array) {
				target_hash->nApplyCount--;
			}
			zend_hash_move_forward(target_hash);
		}
	}
}