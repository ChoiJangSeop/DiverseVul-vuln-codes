SPL_METHOD(SplDoublyLinkedList, push)
{
	zval *value;
	spl_dllist_object *intern;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &value) == FAILURE) {
		return;
	}

	SEPARATE_ARG_IF_REF(value);

	intern = (spl_dllist_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
	spl_ptr_llist_push(intern->llist, value TSRMLS_CC);

	RETURN_TRUE;
} 