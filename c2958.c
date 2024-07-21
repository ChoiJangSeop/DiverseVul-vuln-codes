static void spl_array_method(INTERNAL_FUNCTION_PARAMETERS, char *fname, int fname_len, int use_arg) /* {{{ */
{
	spl_array_object *intern = (spl_array_object*)zend_object_store_get_object(getThis() TSRMLS_CC);
	HashTable *aht = spl_array_get_hash_table(intern, 0 TSRMLS_CC);
	zval *tmp, *arg = NULL;
	zval *retval_ptr = NULL;
	
	MAKE_STD_ZVAL(tmp);
	Z_TYPE_P(tmp) = IS_ARRAY;
	Z_ARRVAL_P(tmp) = aht;
	
	if (!use_arg) {
		aht->nApplyCount++;
		zend_call_method(NULL, NULL, NULL, fname, fname_len, &retval_ptr, 1, tmp, NULL TSRMLS_CC);
		aht->nApplyCount--;
	} else if (use_arg == SPL_ARRAY_METHOD_MAY_USER_ARG) {
		if (zend_parse_parameters_ex(ZEND_PARSE_PARAMS_QUIET, ZEND_NUM_ARGS() TSRMLS_CC, "|z", &arg) == FAILURE) {
			Z_TYPE_P(tmp) = IS_NULL;
			zval_ptr_dtor(&tmp);
			zend_throw_exception(spl_ce_BadMethodCallException, "Function expects one argument at most", 0 TSRMLS_CC);
			return;
		}
		aht->nApplyCount++;
		zend_call_method(NULL, NULL, NULL, fname, fname_len, &retval_ptr, arg? 2 : 1, tmp, arg TSRMLS_CC);
		aht->nApplyCount--;
	} else {
		if (ZEND_NUM_ARGS() != 1 || zend_parse_parameters_ex(ZEND_PARSE_PARAMS_QUIET, ZEND_NUM_ARGS() TSRMLS_CC, "z", &arg) == FAILURE) {
			Z_TYPE_P(tmp) = IS_NULL;
			zval_ptr_dtor(&tmp);
			zend_throw_exception(spl_ce_BadMethodCallException, "Function expects exactly one argument", 0 TSRMLS_CC);
			return;
		}
		aht->nApplyCount++;
		zend_call_method(NULL, NULL, NULL, fname, fname_len, &retval_ptr, 2, tmp, arg TSRMLS_CC);
		aht->nApplyCount--;
	}
	Z_TYPE_P(tmp) = IS_NULL; /* we want to destroy the zval, not the hashtable */
	zval_ptr_dtor(&tmp);
	if (retval_ptr) {
		COPY_PZVAL_TO_ZVAL(*return_value, retval_ptr);
	}
} /* }}} */