static void spl_array_set_array(zval *object, spl_array_object *intern, zval **array, long ar_flags, int just_array TSRMLS_DC) {

	if (Z_TYPE_PP(array) == IS_ARRAY) {
		SEPARATE_ZVAL_IF_NOT_REF(array);
	}

	if (Z_TYPE_PP(array) == IS_OBJECT && (Z_OBJ_HT_PP(array) == &spl_handler_ArrayObject || Z_OBJ_HT_PP(array) == &spl_handler_ArrayIterator)) {
		zval_ptr_dtor(&intern->array);
		if (just_array)	{
			spl_array_object *other = (spl_array_object*)zend_object_store_get_object(*array TSRMLS_CC);
			ar_flags = other->ar_flags & ~SPL_ARRAY_INT_MASK;
		}		
		ar_flags |= SPL_ARRAY_USE_OTHER;
		intern->array = *array;
	} else {
		if (Z_TYPE_PP(array) != IS_OBJECT && Z_TYPE_PP(array) != IS_ARRAY) {
			zend_throw_exception(spl_ce_InvalidArgumentException, "Passed variable is not an array or object, using empty array instead", 0 TSRMLS_CC);
			return;
		}
		zval_ptr_dtor(&intern->array);
		intern->array = *array;
	}
	if (object == *array) {
		intern->ar_flags |= SPL_ARRAY_IS_SELF;
		intern->ar_flags &= ~SPL_ARRAY_USE_OTHER;
	} else {
		intern->ar_flags &= ~SPL_ARRAY_IS_SELF;
	}
	intern->ar_flags |= ar_flags;
	Z_ADDREF_P(intern->array);
	if (Z_TYPE_PP(array) == IS_OBJECT) {
		zend_object_get_properties_t handler = Z_OBJ_HANDLER_PP(array, get_properties);
		if ((handler != std_object_handlers.get_properties && handler != spl_array_get_properties)
		|| !spl_array_get_hash_table(intern, 0 TSRMLS_CC)) {
			zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0 TSRMLS_CC, "Overloaded object of type %s is not compatible with %s", Z_OBJCE_PP(array)->name, intern->std.ce->name);
		}
	}

	spl_array_rewind(intern TSRMLS_CC);
}