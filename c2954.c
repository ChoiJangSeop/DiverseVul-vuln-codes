static void spl_array_unset_dimension_ex(int check_inherited, zval *object, zval *offset TSRMLS_DC) /* {{{ */
{
	spl_array_object *intern = (spl_array_object*)zend_object_store_get_object(object TSRMLS_CC);
	long index;
	HashTable *ht;

	if (check_inherited && intern->fptr_offset_del) {
		SEPARATE_ARG_IF_REF(offset);
		zend_call_method_with_1_params(&object, Z_OBJCE_P(object), &intern->fptr_offset_del, "offsetUnset", NULL, offset);
		zval_ptr_dtor(&offset);
		return;
	}

	switch(Z_TYPE_P(offset)) {
	case IS_STRING:
		ht = spl_array_get_hash_table(intern, 0 TSRMLS_CC);
		if (ht->nApplyCount > 0) {
			zend_error(E_WARNING, "Modification of ArrayObject during sorting is prohibited");
			return;
		}
		if (ht == &EG(symbol_table)) {
			if (zend_delete_global_variable(Z_STRVAL_P(offset), Z_STRLEN_P(offset) TSRMLS_CC)) {
				zend_error(E_NOTICE,"Undefined index: %s", Z_STRVAL_P(offset));
			}
		} else {
			if (zend_symtable_del(ht, Z_STRVAL_P(offset), Z_STRLEN_P(offset)+1) == FAILURE) {
				zend_error(E_NOTICE,"Undefined index: %s", Z_STRVAL_P(offset));
			} else {
				spl_array_object *obj = intern;

				while (1) {
					if ((obj->ar_flags & SPL_ARRAY_IS_SELF) != 0) {
						break;
					} else if (Z_TYPE_P(obj->array) == IS_OBJECT) {
					    if ((obj->ar_flags & SPL_ARRAY_USE_OTHER) == 0) {
							obj = (spl_array_object*)zend_object_store_get_object(obj->array TSRMLS_CC);
					    	break;
						} else {
							obj = (spl_array_object*)zend_object_store_get_object(obj->array TSRMLS_CC);
					    }
					} else {
						obj = NULL;
						break;
					}
				}
				if (obj) {
					zend_property_info *property_info = zend_get_property_info(obj->std.ce, offset, 1 TSRMLS_CC);

					if (property_info &&
					    (property_info->flags & ZEND_ACC_STATIC) == 0 &&
					    property_info->offset >= 0) {
					    obj->std.properties_table[property_info->offset] = NULL;
					}
				}
			}			
		}
		break;
	case IS_DOUBLE:
	case IS_RESOURCE:
	case IS_BOOL: 
	case IS_LONG: 
		if (offset->type == IS_DOUBLE) {
			index = (long)Z_DVAL_P(offset);
		} else {
			index = Z_LVAL_P(offset);
		}
		ht = spl_array_get_hash_table(intern, 0 TSRMLS_CC);
		if (ht->nApplyCount > 0) {
			zend_error(E_WARNING, "Modification of ArrayObject during sorting is prohibited");
			return;
		}
		if (zend_hash_index_del(ht, index) == FAILURE) {
			zend_error(E_NOTICE,"Undefined offset: %ld", Z_LVAL_P(offset));
		}
		break;
	default:
		zend_error(E_WARNING, "Illegal offset type");
		return;
	}
	spl_hash_verify_pos(intern TSRMLS_CC); /* call rewind on FAILURE */
} /* }}} */