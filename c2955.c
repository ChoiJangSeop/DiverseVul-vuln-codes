void spl_array_iterator_append(zval *object, zval *append_value TSRMLS_DC) /* {{{ */
{
	spl_array_object *intern = (spl_array_object*)zend_object_store_get_object(object TSRMLS_CC);
	HashTable *aht = spl_array_get_hash_table(intern, 0 TSRMLS_CC);

	if (!aht) {
		php_error_docref(NULL TSRMLS_CC, E_NOTICE, "Array was modified outside object and is no longer an array");
		return;
	}
	
	if (Z_TYPE_P(intern->array) == IS_OBJECT) {
		php_error_docref(NULL TSRMLS_CC, E_RECOVERABLE_ERROR, "Cannot append properties to objects, use %s::offsetSet() instead", Z_OBJCE_P(object)->name);
		return;
	}

	spl_array_write_dimension(object, NULL, append_value TSRMLS_CC);
	if (!intern->pos) {
		spl_array_set_pos(intern, aht->pListTail);
	}
} /* }}} */