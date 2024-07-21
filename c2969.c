static zval *spl_array_read_dimension_ex(int check_inherited, zval *object, zval *offset, int type TSRMLS_DC) /* {{{ */
{
	zval **ret;

	if (check_inherited) {
		spl_array_object *intern = (spl_array_object*)zend_object_store_get_object(object TSRMLS_CC);
		if (intern->fptr_offset_get) {
			zval *rv;
			if (!offset) {
				ALLOC_INIT_ZVAL(offset);
			} else {
				SEPARATE_ARG_IF_REF(offset);
			}
			zend_call_method_with_1_params(&object, Z_OBJCE_P(object), &intern->fptr_offset_get, "offsetGet", &rv, offset);	
			zval_ptr_dtor(&offset);
			if (rv) {
				zval_ptr_dtor(&intern->retval);
				MAKE_STD_ZVAL(intern->retval);
				ZVAL_ZVAL(intern->retval, rv, 1, 1);
				return intern->retval;
			}
			return EG(uninitialized_zval_ptr);
		}
	}
	ret = spl_array_get_dimension_ptr_ptr(check_inherited, object, offset, type TSRMLS_CC);

	/* When in a write context,
	 * ZE has to be fooled into thinking this is in a reference set
	 * by separating (if necessary) and returning as an is_ref=1 zval (even if refcount == 1) */
	if ((type == BP_VAR_W || type == BP_VAR_RW || type == BP_VAR_UNSET) && !Z_ISREF_PP(ret) && ret != &EG(uninitialized_zval_ptr)) {
		if (Z_REFCOUNT_PP(ret) > 1) {
			zval *newval;

			/* Separate */
			MAKE_STD_ZVAL(newval);
			*newval = **ret;
			zval_copy_ctor(newval);
			Z_SET_REFCOUNT_P(newval, 1);

			/* Replace */
			Z_DELREF_PP(ret);
			*ret = newval;
		}

		Z_SET_ISREF_PP(ret);
	}

	return *ret;
} /* }}} */