static zval **php_zip_get_property_ptr_ptr(zval *object, zval *member, const zend_literal *key TSRMLS_DC) /* {{{ */
{
	ze_zip_object *obj;
	zval tmp_member;
	zval **retval = NULL;

	zip_prop_handler *hnd;
	zend_object_handlers *std_hnd;
	int ret;

	if (member->type != IS_STRING) {
		tmp_member = *member;
		zval_copy_ctor(&tmp_member);
		convert_to_string(&tmp_member);
		member = &tmp_member;
		key = NULL;
	}

	ret = FAILURE;
	obj = (ze_zip_object *)zend_objects_get_address(object TSRMLS_CC);

	if (obj->prop_handler != NULL) {
		if (key) {
			ret = zend_hash_quick_find(obj->prop_handler, Z_STRVAL_P(member), Z_STRLEN_P(member)+1, key->hash_value, (void **) &hnd);
		} else {	
			ret = zend_hash_find(obj->prop_handler, Z_STRVAL_P(member), Z_STRLEN_P(member)+1, (void **) &hnd);
		}
	}


	if (ret == FAILURE) {
		std_hnd = zend_get_std_object_handlers();
		retval = std_hnd->get_property_ptr_ptr(object, member, key TSRMLS_CC);
	}

	if (member == &tmp_member) {
		zval_dtor(member);
	}
	return retval;
}