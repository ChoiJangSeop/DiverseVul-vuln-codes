SPL_METHOD(SplDoublyLinkedList, rewind)
{
	spl_dllist_object *intern = (spl_dllist_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
	
	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}

	spl_dllist_it_helper_rewind(&intern->traverse_pointer, &intern->traverse_position, intern->llist, intern->flags TSRMLS_CC);
}