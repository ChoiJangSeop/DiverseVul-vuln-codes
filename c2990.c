static void php_var_serialize_intern(smart_str *buf, zval *struc, HashTable *var_hash TSRMLS_DC) /* {{{ */
{
	int i;
	ulong *var_already;
	HashTable *myht;

	if (EG(exception)) {
		return;
	}

	if (var_hash && php_add_var_hash(var_hash, struc, (void *) &var_already TSRMLS_CC) == FAILURE) {
		if (Z_ISREF_P(struc)) {
			smart_str_appendl(buf, "R:", 2);
			smart_str_append_long(buf, (long)*var_already);
			smart_str_appendc(buf, ';');
			return;
		} else if (Z_TYPE_P(struc) == IS_OBJECT) {
			smart_str_appendl(buf, "r:", 2);
			smart_str_append_long(buf, (long)*var_already);
			smart_str_appendc(buf, ';');
			return;
		}
	}

	switch (Z_TYPE_P(struc)) {
		case IS_BOOL:
			smart_str_appendl(buf, "b:", 2);
			smart_str_append_long(buf, Z_LVAL_P(struc));
			smart_str_appendc(buf, ';');
			return;

		case IS_NULL:
			smart_str_appendl(buf, "N;", 2);
			return;

		case IS_LONG:
			php_var_serialize_long(buf, Z_LVAL_P(struc));
			return;

		case IS_DOUBLE: {
				char *s;

				smart_str_appendl(buf, "d:", 2);
				s = (char *) safe_emalloc(PG(serialize_precision), 1, MAX_LENGTH_OF_DOUBLE + 1);
				php_gcvt(Z_DVAL_P(struc), PG(serialize_precision), '.', 'E', s);
				smart_str_appends(buf, s);
				smart_str_appendc(buf, ';');
				efree(s);
				return;
			}

		case IS_STRING:
			php_var_serialize_string(buf, Z_STRVAL_P(struc), Z_STRLEN_P(struc));
			return;

		case IS_OBJECT: {
				zval *retval_ptr = NULL;
				zval fname;
				int res;
				zend_class_entry *ce = NULL;

				if (Z_OBJ_HT_P(struc)->get_class_entry) {
					ce = Z_OBJCE_P(struc);
				}

				if (ce && ce->serialize != NULL) {
					/* has custom handler */
					unsigned char *serialized_data = NULL;
					zend_uint serialized_length;

					if (ce->serialize(struc, &serialized_data, &serialized_length, (zend_serialize_data *)var_hash TSRMLS_CC) == SUCCESS) {
						smart_str_appendl(buf, "C:", 2);
						smart_str_append_long(buf, (int)Z_OBJCE_P(struc)->name_length);
						smart_str_appendl(buf, ":\"", 2);
						smart_str_appendl(buf, Z_OBJCE_P(struc)->name, Z_OBJCE_P(struc)->name_length);
						smart_str_appendl(buf, "\":", 2);

						smart_str_append_long(buf, (int)serialized_length);
						smart_str_appendl(buf, ":{", 2);
						smart_str_appendl(buf, serialized_data, serialized_length);
						smart_str_appendc(buf, '}');
					} else {
						smart_str_appendl(buf, "N;", 2);
					}
					if (serialized_data) {
						efree(serialized_data);
					}
					return;
				}

				if (ce && ce != PHP_IC_ENTRY && zend_hash_exists(&ce->function_table, "__sleep", sizeof("__sleep"))) {
					INIT_PZVAL(&fname);
					ZVAL_STRINGL(&fname, "__sleep", sizeof("__sleep") - 1, 0);
					BG(serialize_lock)++;
					res = call_user_function_ex(CG(function_table), &struc, &fname, &retval_ptr, 0, 0, 1, NULL TSRMLS_CC);
					BG(serialize_lock)--;
                    
					if (EG(exception)) {
						if (retval_ptr) {
							zval_ptr_dtor(&retval_ptr);
						}
						return;
					}

					if (res == SUCCESS) {
						if (retval_ptr) {
							if (HASH_OF(retval_ptr)) {
								php_var_serialize_class(buf, struc, retval_ptr, var_hash TSRMLS_CC);
							} else {
								php_error_docref(NULL TSRMLS_CC, E_NOTICE, "__sleep should return an array only containing the names of instance-variables to serialize");
								/* we should still add element even if it's not OK,
								 * since we already wrote the length of the array before */
								smart_str_appendl(buf,"N;", 2);
							}
							zval_ptr_dtor(&retval_ptr);
						}
						return;
					}
				}

				if (retval_ptr) {
					zval_ptr_dtor(&retval_ptr);
				}
				/* fall-through */
			}
		case IS_ARRAY: {
			zend_bool incomplete_class = 0;
			if (Z_TYPE_P(struc) == IS_ARRAY) {
				smart_str_appendl(buf, "a:", 2);
				myht = HASH_OF(struc);
			} else {
				incomplete_class = php_var_serialize_class_name(buf, struc TSRMLS_CC);
				myht = Z_OBJPROP_P(struc);
			}
			/* count after serializing name, since php_var_serialize_class_name
			 * changes the count if the variable is incomplete class */
			i = myht ? zend_hash_num_elements(myht) : 0;
			if (i > 0 && incomplete_class) {
				--i;
			}
			smart_str_append_long(buf, i);
			smart_str_appendl(buf, ":{", 2);
			if (i > 0) {
				char *key;
				zval **data;
				ulong index;
				uint key_len;
				HashPosition pos;

				zend_hash_internal_pointer_reset_ex(myht, &pos);
				for (;; zend_hash_move_forward_ex(myht, &pos)) {
					i = zend_hash_get_current_key_ex(myht, &key, &key_len, &index, 0, &pos);
					if (i == HASH_KEY_NON_EXISTANT) {
						break;
					}
					if (incomplete_class && strcmp(key, MAGIC_MEMBER) == 0) {
						continue;
					}

					switch (i) {
						case HASH_KEY_IS_LONG:
							php_var_serialize_long(buf, index);
							break;
						case HASH_KEY_IS_STRING:
							php_var_serialize_string(buf, key, key_len - 1);
							break;
					}

					/* we should still add element even if it's not OK,
					 * since we already wrote the length of the array before */
					if (zend_hash_get_current_data_ex(myht, (void **) &data, &pos) != SUCCESS
						|| !data
						|| data == &struc
						|| (Z_TYPE_PP(data) == IS_ARRAY && Z_ARRVAL_PP(data)->nApplyCount > 1)
					) {
						smart_str_appendl(buf, "N;", 2);
					} else {
						if (Z_TYPE_PP(data) == IS_ARRAY) {
							Z_ARRVAL_PP(data)->nApplyCount++;
						}
						php_var_serialize_intern(buf, *data, var_hash TSRMLS_CC);
						if (Z_TYPE_PP(data) == IS_ARRAY) {
							Z_ARRVAL_PP(data)->nApplyCount--;
						}
					}
				}
			}
			smart_str_appendc(buf, '}');
			return;
		}
		default:
			smart_str_appendl(buf, "i:0;", 4);
			return;
	}
}