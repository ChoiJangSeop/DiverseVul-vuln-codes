static void spl_array_it_move_forward(zend_object_iterator *iter TSRMLS_DC) /* {{{ */
{
	spl_array_it       *iterator = (spl_array_it *)iter;
	spl_array_object   *object   = iterator->object;
	HashTable          *aht      = spl_array_get_hash_table(object, 0 TSRMLS_CC);

	if (object->ar_flags & SPL_ARRAY_OVERLOADED_NEXT) {
		zend_user_it_move_forward(iter TSRMLS_CC);
	} else {
		zend_user_it_invalidate_current(iter TSRMLS_CC);
		if (!aht) {
			php_error_docref(NULL TSRMLS_CC, E_NOTICE, "ArrayIterator::current(): Array was modified outside object and is no longer an array");
			return;
		}
	
		if ((object->ar_flags & SPL_ARRAY_IS_REF) && spl_hash_verify_pos_ex(object, aht TSRMLS_CC) == FAILURE) {
			php_error_docref(NULL TSRMLS_CC, E_NOTICE, "ArrayIterator::next(): Array was modified outside object and internal position is no longer valid");
		} else {
			spl_array_next_no_verify(object, aht TSRMLS_CC);
		}
	}
}