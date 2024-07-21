static char *php_replace_in_subject(zval *regex, zval *replace, zval **subject, int *result_len, int limit, int is_callable_replace, int *replace_count TSRMLS_DC)
{
	zval		**regex_entry,
				**replace_entry = NULL,
				 *replace_value,
				  empty_replace;
	char		*subject_value,
				*result;
	int			 subject_len;

	/* Make sure we're dealing with strings. */	
	convert_to_string_ex(subject);
	/* FIXME: This might need to be changed to STR_EMPTY_ALLOC(). Check if this zval could be dtor()'ed somehow */
	ZVAL_STRINGL(&empty_replace, "", 0, 0);
	
	/* If regex is an array */
	if (Z_TYPE_P(regex) == IS_ARRAY) {
		/* Duplicate subject string for repeated replacement */
		subject_value = estrndup(Z_STRVAL_PP(subject), Z_STRLEN_PP(subject));
		subject_len = Z_STRLEN_PP(subject);
		*result_len = subject_len;
		
		zend_hash_internal_pointer_reset(Z_ARRVAL_P(regex));

		replace_value = replace;
		if (Z_TYPE_P(replace) == IS_ARRAY && !is_callable_replace)
			zend_hash_internal_pointer_reset(Z_ARRVAL_P(replace));

		/* For each entry in the regex array, get the entry */
		while (zend_hash_get_current_data(Z_ARRVAL_P(regex), (void **)&regex_entry) == SUCCESS) {
			/* Make sure we're dealing with strings. */	
			convert_to_string_ex(regex_entry);
		
			/* If replace is an array and not a callable construct */
			if (Z_TYPE_P(replace) == IS_ARRAY && !is_callable_replace) {
				/* Get current entry */
				if (zend_hash_get_current_data(Z_ARRVAL_P(replace), (void **)&replace_entry) == SUCCESS) {
					if (!is_callable_replace) {
						convert_to_string_ex(replace_entry);
					}
					replace_value = *replace_entry;
					zend_hash_move_forward(Z_ARRVAL_P(replace));
				} else {
					/* We've run out of replacement strings, so use an empty one */
					replace_value = &empty_replace;
				}
			}
			
			/* Do the actual replacement and put the result back into subject_value
			   for further replacements. */
			if ((result = php_pcre_replace(Z_STRVAL_PP(regex_entry),
										   Z_STRLEN_PP(regex_entry),
										   subject_value,
										   subject_len,
										   replace_value,
										   is_callable_replace,
										   result_len,
										   limit,
										   replace_count TSRMLS_CC)) != NULL) {
				efree(subject_value);
				subject_value = result;
				subject_len = *result_len;
			} else {
				efree(subject_value);
				return NULL;
			}

			zend_hash_move_forward(Z_ARRVAL_P(regex));
		}

		return subject_value;
	} else {
		result = php_pcre_replace(Z_STRVAL_P(regex),
								  Z_STRLEN_P(regex),
								  Z_STRVAL_PP(subject),
								  Z_STRLEN_PP(subject),
								  replace,
								  is_callable_replace,
								  result_len,
								  limit,
								  replace_count TSRMLS_CC);
		return result;
	}
}