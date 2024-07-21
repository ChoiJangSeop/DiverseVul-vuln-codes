SPL_METHOD(SplDoublyLinkedList, current)
{
	spl_dllist_object     *intern  = (spl_dllist_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
	spl_ptr_llist_element *element = intern->traverse_pointer;
	
	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}

	if (element == NULL || element->data == NULL) {
		RETURN_NULL();
	} else {
		zval *data    = (zval *)element->data;
		RETURN_ZVAL(data, 1, 0);
	}
}