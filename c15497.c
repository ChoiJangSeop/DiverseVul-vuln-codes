static zval **spl_array_get_dimension_ptr_ptr(int check_inherited, zval *object, zval *offset, int type TSRMLS_DC) /* {{{ */
{
	spl_array_object *intern = (spl_array_object*)zend_object_store_get_object(object TSRMLS_CC);
	zval **retval;
	char *key;
	uint len;
	long index;
	HashTable *ht = spl_array_get_hash_table(intern, 0 TSRMLS_CC);

	if (!offset) {
		return &EG(uninitialized_zval_ptr);
	}

	if ((type == BP_VAR_W || type == BP_VAR_RW) && (ht->nApplyCount > 0)) {
		zend_error(E_WARNING, "Modification of ArrayObject during sorting is prohibited");
		return &EG(error_zval_ptr);;
	}

	switch (Z_TYPE_P(offset)) {
	case IS_STRING:
		key = Z_STRVAL_P(offset);
		len = Z_STRLEN_P(offset) + 1;
string_offest:
		if (zend_symtable_find(ht, key, len, (void **) &retval) == FAILURE) {
			switch (type) {
				case BP_VAR_R:
					zend_error(E_NOTICE, "Undefined index: %s", key);
				case BP_VAR_UNSET:
				case BP_VAR_IS:
					retval = &EG(uninitialized_zval_ptr);
					break;
				case BP_VAR_RW:
					zend_error(E_NOTICE,"Undefined index: %s", key);
				case BP_VAR_W: {
				    zval *value;
				    ALLOC_INIT_ZVAL(value);
				    zend_symtable_update(ht, key, len, (void**)&value, sizeof(void*), (void **)&retval);
				}
			}
		}
		return retval;
	case IS_NULL:
		key = "";
		len = 1;
		goto string_offest;
	case IS_RESOURCE:
		zend_error(E_STRICT, "Resource ID#%ld used as offset, casting to integer (%ld)", Z_LVAL_P(offset), Z_LVAL_P(offset));
	case IS_DOUBLE:
	case IS_BOOL:
	case IS_LONG:
		if (offset->type == IS_DOUBLE) {
			index = (long)Z_DVAL_P(offset);
		} else {
			index = Z_LVAL_P(offset);
		}
		if (zend_hash_index_find(ht, index, (void **) &retval) == FAILURE) {
			switch (type) {
				case BP_VAR_R:
					zend_error(E_NOTICE, "Undefined offset: %ld", index);
				case BP_VAR_UNSET:
				case BP_VAR_IS:
					retval = &EG(uninitialized_zval_ptr);
					break;
				case BP_VAR_RW:
					zend_error(E_NOTICE, "Undefined offset: %ld", index);
				case BP_VAR_W: {
				    zval *value;
				    ALLOC_INIT_ZVAL(value);
					zend_hash_index_update(ht, index, (void**)&value, sizeof(void*), (void **)&retval);
			   }
			}
		}
		return retval;
	default:
		zend_error(E_WARNING, "Illegal offset type");
		return (type == BP_VAR_W || type == BP_VAR_RW) ?
			&EG(error_zval_ptr) : &EG(uninitialized_zval_ptr);
	}
} /* }}} */