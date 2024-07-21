PS_SERIALIZER_DECODE_FUNC(wddx)
{
	zval *retval;
	zval **ent;
	char *key;
	uint key_length;
	char tmp[128];
	ulong idx;
	int hash_type;
	int ret;

	if (vallen == 0) {
		return SUCCESS;
	}
	
	MAKE_STD_ZVAL(retval);

	if ((ret = php_wddx_deserialize_ex((char *)val, vallen, retval)) == SUCCESS) {

		for (zend_hash_internal_pointer_reset(Z_ARRVAL_P(retval));
			 zend_hash_get_current_data(Z_ARRVAL_P(retval), (void **) &ent) == SUCCESS;
			 zend_hash_move_forward(Z_ARRVAL_P(retval))) {
			hash_type = zend_hash_get_current_key_ex(Z_ARRVAL_P(retval), &key, &key_length, &idx, 0, NULL);

			switch (hash_type) {
				case HASH_KEY_IS_LONG:
					key_length = slprintf(tmp, sizeof(tmp), "%ld", idx) + 1;
					key = tmp;
					/* fallthru */
				case HASH_KEY_IS_STRING:
					php_set_session_var(key, key_length-1, *ent, NULL TSRMLS_CC);
					PS_ADD_VAR(key);
			}
		}
	}

	zval_ptr_dtor(&retval);

	return ret;
}