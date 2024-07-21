static void spl_array_write_dimension_ex(int check_inherited, zval *object, zval *offset, zval *value TSRMLS_DC) /* {{{ */
{
	spl_array_object *intern = (spl_array_object*)zend_object_store_get_object(object TSRMLS_CC);
	long index;
	HashTable *ht;

	if (check_inherited && intern->fptr_offset_set) {
		if (!offset) {
			ALLOC_INIT_ZVAL(offset);
		} else {
			SEPARATE_ARG_IF_REF(offset);
		}
		zend_call_method_with_2_params(&object, Z_OBJCE_P(object), &intern->fptr_offset_set, "offsetSet", NULL, offset, value);
		zval_ptr_dtor(&offset);
		return;
	}
	
	if (!offset) {
		ht = spl_array_get_hash_table(intern, 0 TSRMLS_CC);
		if (ht->nApplyCount > 0) {
			zend_error(E_WARNING, "Modification of ArrayObject during sorting is prohibited");
			return;
		}
		Z_ADDREF_P(value);
		zend_hash_next_index_insert(ht, (void**)&value, sizeof(void*), NULL);
		return;
	}
	switch(Z_TYPE_P(offset)) {
	case IS_STRING:
		ht = spl_array_get_hash_table(intern, 0 TSRMLS_CC);
		if (ht->nApplyCount > 0) {
			zend_error(E_WARNING, "Modification of ArrayObject during sorting is prohibited");
			return;
		}
		Z_ADDREF_P(value);
		zend_symtable_update(ht, Z_STRVAL_P(offset), Z_STRLEN_P(offset)+1, (void**)&value, sizeof(void*), NULL);
		return;
	case IS_DOUBLE:
	case IS_RESOURCE:
	case IS_BOOL: 
	case IS_LONG: 
		ht = spl_array_get_hash_table(intern, 0 TSRMLS_CC);
		if (ht->nApplyCount > 0) {
			zend_error(E_WARNING, "Modification of ArrayObject during sorting is prohibited");
			return;
		}
		if (offset->type == IS_DOUBLE) {
			index = (long)Z_DVAL_P(offset);
		} else {
			index = Z_LVAL_P(offset);
		}
		Z_ADDREF_P(value);
		zend_hash_index_update(ht, index, (void**)&value, sizeof(void*), NULL);
		return;
	case IS_NULL:
		ht = spl_array_get_hash_table(intern, 0 TSRMLS_CC);
		if (ht->nApplyCount > 0) {
			zend_error(E_WARNING, "Modification of ArrayObject during sorting is prohibited");
			return;
		}
		Z_ADDREF_P(value);
		zend_hash_next_index_insert(ht, (void**)&value, sizeof(void*), NULL);
		return;
	default:
		zend_error(E_WARNING, "Illegal offset type");
		return;
	}
} /* }}} */