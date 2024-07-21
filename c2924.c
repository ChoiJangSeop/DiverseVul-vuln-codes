static void spl_multiple_iterator_get_all(spl_SplObjectStorage *intern, int get_type, zval *return_value TSRMLS_DC) /* {{{ */
{
	spl_SplObjectStorageElement *element;
	zval                        *it, *retval = NULL;
	int                          valid = 1, num_elements;

	num_elements = zend_hash_num_elements(&intern->storage);
	if (num_elements < 1) {
		RETURN_FALSE;
	}

	array_init_size(return_value, num_elements);
	
	zend_hash_internal_pointer_reset_ex(&intern->storage, &intern->pos);
	while (zend_hash_get_current_data_ex(&intern->storage, (void**)&element, &intern->pos) == SUCCESS && !EG(exception)) {
		it = element->obj;
		zend_call_method_with_0_params(&it, Z_OBJCE_P(it), &Z_OBJCE_P(it)->iterator_funcs.zf_valid, "valid", &retval);

		if (retval) {
			valid = Z_LVAL_P(retval);
			zval_ptr_dtor(&retval);
		} else {
			valid = 0;
		}

		if (valid) {
			if (SPL_MULTIPLE_ITERATOR_GET_ALL_CURRENT == get_type) {
				zend_call_method_with_0_params(&it, Z_OBJCE_P(it), &Z_OBJCE_P(it)->iterator_funcs.zf_current, "current", &retval);
			} else {
				zend_call_method_with_0_params(&it, Z_OBJCE_P(it), &Z_OBJCE_P(it)->iterator_funcs.zf_key,     "key",     &retval);
			}
			if (!retval) {
				zend_throw_exception(spl_ce_RuntimeException, "Failed to call sub iterator method", 0 TSRMLS_CC);
				return;
			}
		} else if (intern->flags & MIT_NEED_ALL) {
			if (SPL_MULTIPLE_ITERATOR_GET_ALL_CURRENT == get_type) {
				zend_throw_exception(spl_ce_RuntimeException, "Called current() with non valid sub iterator", 0 TSRMLS_CC);
			} else {
				zend_throw_exception(spl_ce_RuntimeException, "Called key() with non valid sub iterator", 0 TSRMLS_CC);
			}
			return;
		} else {
			ALLOC_INIT_ZVAL(retval);
		}

		if (intern->flags & MIT_KEYS_ASSOC) {
			switch (Z_TYPE_P(element->inf)) {
				case IS_LONG:
					add_index_zval(return_value, Z_LVAL_P(element->inf), retval);
					break;
				case IS_STRING:
					add_assoc_zval_ex(return_value, Z_STRVAL_P(element->inf), Z_STRLEN_P(element->inf)+1U, retval);
					break;
				default:
					zval_ptr_dtor(&retval);
					zend_throw_exception(spl_ce_InvalidArgumentException, "Sub-Iterator is associated with NULL", 0 TSRMLS_CC);
					return;
			}
		} else {
			add_next_index_zval(return_value, retval);
		}

		zend_hash_move_forward_ex(&intern->storage, &intern->pos);
	}
}