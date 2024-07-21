static int spl_array_it_get_current_key(zend_object_iterator *iter, char **str_key, uint *str_key_len, ulong *int_key TSRMLS_DC) /* {{{ */
{
	spl_array_it       *iterator = (spl_array_it *)iter;
	spl_array_object   *object   = iterator->object;
	HashTable          *aht      = spl_array_get_hash_table(object, 0 TSRMLS_CC);

	if (object->ar_flags & SPL_ARRAY_OVERLOADED_KEY) {
		return zend_user_it_get_current_key(iter, str_key, str_key_len, int_key TSRMLS_CC);
	} else {
		if (spl_array_object_verify_pos_ex(object, aht, "ArrayIterator::current(): " TSRMLS_CC) == FAILURE) {
			return HASH_KEY_NON_EXISTANT;
		}
	
		return zend_hash_get_current_key_ex(aht, str_key, str_key_len, int_key, 1, &object->pos);
	}
}