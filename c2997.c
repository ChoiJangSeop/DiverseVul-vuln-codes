static PHP_FUNCTION(session_set_save_handler)
{
	zval ***args = NULL;
	int i, num_args, argc = ZEND_NUM_ARGS();
	char *name;

	if (PS(session_status) != php_session_none) {
		RETURN_FALSE;
	}

	if (argc != 1 && argc != 2 && argc != 6) {
		WRONG_PARAM_COUNT;
	}

	if (argc <= 2) {
		zval *obj = NULL, *callback = NULL;
		zend_uint func_name_len;
		char *func_name;
		HashPosition pos;
		zend_function *default_mptr, *current_mptr;
		ulong func_index;
		php_shutdown_function_entry shutdown_function_entry;
		zend_bool register_shutdown = 1;

		if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O|b", &obj, php_session_iface_entry, &register_shutdown) == FAILURE) {
			RETURN_FALSE;
		}

		/* Find implemented methods */
		zend_hash_internal_pointer_reset_ex(&php_session_class_entry->function_table, &pos);
		i = 0;
		while (zend_hash_get_current_data_ex(&php_session_class_entry->function_table, (void **) &default_mptr, &pos) == SUCCESS) {
			zend_hash_get_current_key_ex(&php_session_class_entry->function_table, &func_name, &func_name_len, &func_index, 0, &pos);

			if (zend_hash_find(&Z_OBJCE_P(obj)->function_table, func_name, func_name_len, (void **)&current_mptr) == SUCCESS) {
				if (PS(mod_user_names).names[i] != NULL) {
					zval_ptr_dtor(&PS(mod_user_names).names[i]);
				}

				MAKE_STD_ZVAL(callback);
				array_init_size(callback, 2);
				Z_ADDREF_P(obj);
				add_next_index_zval(callback, obj);
				add_next_index_stringl(callback, func_name, func_name_len - 1, 1);
				PS(mod_user_names).names[i] = callback;
			} else {
				php_error_docref(NULL TSRMLS_CC, E_ERROR, "Session handler's function table is corrupt");
				RETURN_FALSE;
			}

			zend_hash_move_forward_ex(&php_session_class_entry->function_table, &pos);
			++i;
		}

		if (register_shutdown) {
			/* create shutdown function */
			shutdown_function_entry.arg_count = 1;
			shutdown_function_entry.arguments = (zval **) safe_emalloc(sizeof(zval *), 1, 0);

			MAKE_STD_ZVAL(callback);
			ZVAL_STRING(callback, "session_register_shutdown", 1);
			shutdown_function_entry.arguments[0] = callback;

			/* add shutdown function, removing the old one if it exists */
			if (!register_user_shutdown_function("session_shutdown", sizeof("session_shutdown"), &shutdown_function_entry TSRMLS_CC)) {
				zval_ptr_dtor(&callback);
				efree(shutdown_function_entry.arguments);
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to register session shutdown function");
				RETURN_FALSE;
			}
		} else {
			/* remove shutdown function */
			remove_user_shutdown_function("session_shutdown", sizeof("session_shutdown") TSRMLS_CC);
		}

		if (PS(mod) && PS(session_status) == php_session_none && PS(mod) != &ps_mod_user) {
			zend_alter_ini_entry("session.save_handler", sizeof("session.save_handler"), "user", sizeof("user")-1, PHP_INI_USER, PHP_INI_STAGE_RUNTIME);
		}

		RETURN_TRUE;
	}

	if (zend_parse_parameters(argc TSRMLS_CC, "+", &args, &num_args) == FAILURE) {
		return;
	}

	/* remove shutdown function */
	remove_user_shutdown_function("session_shutdown", sizeof("session_shutdown") TSRMLS_CC);

	for (i = 0; i < 6; i++) {
		if (!zend_is_callable(*args[i], 0, &name TSRMLS_CC)) {
			efree(args);
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Argument %d is not a valid callback", i+1);
			efree(name);
			RETURN_FALSE;
		}
		efree(name);
	}
	
	if (PS(mod) && PS(mod) != &ps_mod_user) {
		zend_alter_ini_entry("session.save_handler", sizeof("session.save_handler"), "user", sizeof("user")-1, PHP_INI_USER, PHP_INI_STAGE_RUNTIME);
	}

	for (i = 0; i < 6; i++) {
		if (PS(mod_user_names).names[i] != NULL) {
			zval_ptr_dtor(&PS(mod_user_names).names[i]);
		}
		Z_ADDREF_PP(args[i]);
		PS(mod_user_names).names[i] = *args[i];
	}

	efree(args);
	RETURN_TRUE;
}