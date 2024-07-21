SPL_METHOD(SplDoublyLinkedList, valid)
{
	spl_dllist_object *intern = (spl_dllist_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
	
	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}

	RETURN_BOOL(intern->traverse_pointer != NULL);
}