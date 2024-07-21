SPL_METHOD(SplObjectStorage, serialize)
{
	spl_SplObjectStorage *intern = (spl_SplObjectStorage*)zend_object_store_get_object(getThis() TSRMLS_CC);

	spl_SplObjectStorageElement *element;
	zval members, *pmembers, *flags;
	HashPosition      pos;
	php_serialize_data_t var_hash;
	smart_str buf = {0};

	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}

	PHP_VAR_SERIALIZE_INIT(var_hash);
	
	/* storage */
	smart_str_appendl(&buf, "x:", 2);
	MAKE_STD_ZVAL(flags);
	ZVAL_LONG(flags, zend_hash_num_elements(&intern->storage));
	php_var_serialize(&buf, &flags, &var_hash TSRMLS_CC);
	zval_ptr_dtor(&flags);

	zend_hash_internal_pointer_reset_ex(&intern->storage, &pos);

	while(zend_hash_has_more_elements_ex(&intern->storage, &pos) == SUCCESS) {
		if (zend_hash_get_current_data_ex(&intern->storage, (void**)&element, &pos) == FAILURE) {
			smart_str_free(&buf);
			PHP_VAR_SERIALIZE_DESTROY(var_hash);
			RETURN_NULL();
		}
		php_var_serialize(&buf, &element->obj, &var_hash TSRMLS_CC);
		smart_str_appendc(&buf, ',');
		php_var_serialize(&buf, &element->inf, &var_hash TSRMLS_CC);
		smart_str_appendc(&buf, ';');
		zend_hash_move_forward_ex(&intern->storage, &pos);
	}

	/* members */
	smart_str_appendl(&buf, "m:", 2);
	INIT_PZVAL(&members);
	Z_ARRVAL(members) = zend_std_get_properties(getThis() TSRMLS_CC);
	Z_TYPE(members) = IS_ARRAY;
	pmembers = &members;
	php_var_serialize(&buf, &pmembers, &var_hash TSRMLS_CC); /* finishes the string */

	/* done */
	PHP_VAR_SERIALIZE_DESTROY(var_hash);

	if (buf.c) {
		RETURN_STRINGL(buf.c, buf.len, 0);
	} else {
		RETURN_NULL();
	}
	
} /* }}} */