SPL_METHOD(SplDoublyLinkedList, offsetUnset)
{
	zval                  *zindex;
	long                   index;
	spl_dllist_object     *intern;
	spl_ptr_llist_element *element;
	spl_ptr_llist         *llist;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &zindex) == FAILURE) {
		return;
	}

	intern = (spl_dllist_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
	index  = spl_offset_convert_to_long(zindex TSRMLS_CC);
	llist  = intern->llist;

	if (index < 0 || index >= intern->llist->count) {
		zend_throw_exception(spl_ce_OutOfRangeException, "Offset out of range", 0 TSRMLS_CC);
		return;
	}

	element = spl_ptr_llist_offset(intern->llist, index, intern->flags & SPL_DLLIST_IT_LIFO);

	if (element != NULL) {
		/* connect the neightbors */
		if (element->prev) {
			element->prev->next = element->next;
		}

		if (element->next) {
			element->next->prev = element->prev;
		}

		/* take care of head/tail */
		if (element == llist->head) {
			llist->head = element->next;
		}

		if (element == llist->tail) {
			llist->tail = element->prev;
		}

		/* finally, delete the element */
		llist->count--;

		if(llist->dtor) {
			llist->dtor(element TSRMLS_CC);
		}

		zval_ptr_dtor((zval **)&element->data);
		element->data = NULL;

		SPL_LLIST_DELREF(element);
	} else {
		zend_throw_exception(spl_ce_OutOfRangeException, "Offset invalid", 0 TSRMLS_CC);
		return;
	}
} /* }}} */