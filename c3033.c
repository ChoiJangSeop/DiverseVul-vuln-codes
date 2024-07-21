static void preg_replace_impl(INTERNAL_FUNCTION_PARAMETERS, int is_callable_replace, int is_filter)
{
	zval		   **regex,
				   **replace,
				   **subject,
				   **subject_entry,
				   **zcount = NULL;
	char			*result;
	int				 result_len;
	int				 limit_val = -1;
	long			limit = -1;
	char			*string_key;
	ulong			 num_key;
	char			*callback_name;
	int				 replace_count=0, old_replace_count;
	
	/* Get function parameters and do error-checking. */
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ZZZ|lZ", &regex, &replace, &subject, &limit, &zcount) == FAILURE) {
		return;
	}
	
	if (!is_callable_replace && Z_TYPE_PP(replace) == IS_ARRAY && Z_TYPE_PP(regex) != IS_ARRAY) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Parameter mismatch, pattern is a string while replacement is an array");
		RETURN_FALSE;
	}

	SEPARATE_ZVAL(replace);
	if (Z_TYPE_PP(replace) != IS_ARRAY && (Z_TYPE_PP(replace) != IS_OBJECT || !is_callable_replace)) {
		convert_to_string_ex(replace);
	}
	if (is_callable_replace) {
		if (!zend_is_callable(*replace, 0, &callback_name TSRMLS_CC)) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Requires argument 2, '%s', to be a valid callback", callback_name);
			efree(callback_name);
			MAKE_COPY_ZVAL(subject, return_value);
			return;
		}
		efree(callback_name);
	}

	SEPARATE_ZVAL(regex);
	SEPARATE_ZVAL(subject);

	if (ZEND_NUM_ARGS() > 3) {
		limit_val = limit;
	}
		
	if (Z_TYPE_PP(regex) != IS_ARRAY)
		convert_to_string_ex(regex);
	
	/* if subject is an array */
	if (Z_TYPE_PP(subject) == IS_ARRAY) {
		array_init(return_value);
		zend_hash_internal_pointer_reset(Z_ARRVAL_PP(subject));

		/* For each subject entry, convert it to string, then perform replacement
		   and add the result to the return_value array. */
		while (zend_hash_get_current_data(Z_ARRVAL_PP(subject), (void **)&subject_entry) == SUCCESS) {
			SEPARATE_ZVAL(subject_entry);
			old_replace_count = replace_count;
			if ((result = php_replace_in_subject(*regex, *replace, subject_entry, &result_len, limit_val, is_callable_replace, &replace_count TSRMLS_CC)) != NULL) {
				if (!is_filter || replace_count > old_replace_count) {
					/* Add to return array */
					switch(zend_hash_get_current_key(Z_ARRVAL_PP(subject), &string_key, &num_key, 0))
					{
					case HASH_KEY_IS_STRING:
						add_assoc_stringl(return_value, string_key, result, result_len, 0);
						break;

					case HASH_KEY_IS_LONG:
						add_index_stringl(return_value, num_key, result, result_len, 0);
						break;
					}
				} else {
					efree(result);
				}
			}
		
			zend_hash_move_forward(Z_ARRVAL_PP(subject));
		}
	} else {	/* if subject is not an array */
		old_replace_count = replace_count;
		if ((result = php_replace_in_subject(*regex, *replace, subject, &result_len, limit_val, is_callable_replace, &replace_count TSRMLS_CC)) != NULL) {
			if (!is_filter || replace_count > old_replace_count) {
				RETVAL_STRINGL(result, result_len, 0);
			} else {
				efree(result);
			}
		}
	}
	if (ZEND_NUM_ARGS() > 4) {
		zval_dtor(*zcount);
		ZVAL_LONG(*zcount, replace_count);
	}
    	
}