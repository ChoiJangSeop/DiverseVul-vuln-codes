SPL_METHOD(Array, getChildren)
{
	zval *object = getThis(), **entry, *flags;
	spl_array_object *intern = (spl_array_object*)zend_object_store_get_object(object TSRMLS_CC);
	HashTable *aht = spl_array_get_hash_table(intern, 0 TSRMLS_CC);
	
	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}

	if (spl_array_object_verify_pos(intern, aht TSRMLS_CC) == FAILURE) {
		return;
	}

	if (zend_hash_get_current_data_ex(aht, (void **) &entry, &intern->pos) == FAILURE) {
		return;
	}

	if (Z_TYPE_PP(entry) == IS_OBJECT) {
		if ((intern->ar_flags & SPL_ARRAY_CHILD_ARRAYS_ONLY) != 0) {
			return;
		}
		if (instanceof_function(Z_OBJCE_PP(entry), Z_OBJCE_P(getThis()) TSRMLS_CC)) {
			RETURN_ZVAL(*entry, 1, 0);
		}
	}

	MAKE_STD_ZVAL(flags);
	ZVAL_LONG(flags, SPL_ARRAY_USE_OTHER | intern->ar_flags);
	spl_instantiate_arg_ex2(Z_OBJCE_P(getThis()), &return_value, 0, *entry, flags TSRMLS_CC);
	zval_ptr_dtor(&flags);
}