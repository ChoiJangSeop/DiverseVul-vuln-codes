static int spl_dllist_object_count_elements(zval *object, long *count TSRMLS_DC) /* {{{ */
{
	spl_dllist_object *intern = (spl_dllist_object*)zend_object_store_get_object(object TSRMLS_CC);

	if (intern->fptr_count) {
		zval *rv;
		zend_call_method_with_0_params(&object, intern->std.ce, &intern->fptr_count, "count", &rv);
		if (rv) {
			zval_ptr_dtor(&intern->retval);
			MAKE_STD_ZVAL(intern->retval);
			ZVAL_ZVAL(intern->retval, rv, 1, 1);
			convert_to_long(intern->retval);
			*count = (long) Z_LVAL_P(intern->retval);
			return SUCCESS;
		}
		*count = 0;
		return FAILURE;
	}

	*count = spl_ptr_llist_count(intern->llist);
	return SUCCESS;
} 