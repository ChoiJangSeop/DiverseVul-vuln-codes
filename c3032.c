static int preg_do_eval(char *eval_str, int eval_str_len, char *subject,
						int *offsets, int count, char **result TSRMLS_DC)
{
	zval		 retval;			/* Return value from evaluation */
	char		*eval_str_end,		/* End of eval string */
				*match,				/* Current match for a backref */
				*esc_match,			/* Quote-escaped match */
				*walk,				/* Used to walk the code string */
				*segment,			/* Start of segment to append while walking */
				 walk_last;			/* Last walked character */
	int			 match_len;			/* Length of the match */
	int			 esc_match_len;		/* Length of the quote-escaped match */
	int			 result_len;		/* Length of the result of the evaluation */
	int			 backref;			/* Current backref */
	char        *compiled_string_description;
	smart_str    code = {0};
	
	eval_str_end = eval_str + eval_str_len;
	walk = segment = eval_str;
	walk_last = 0;
	
	while (walk < eval_str_end) {
		/* If found a backreference.. */
		if ('\\' == *walk || '$' == *walk) {
			smart_str_appendl(&code, segment, walk - segment);
			if (walk_last == '\\') {
				code.c[code.len-1] = *walk++;
				segment = walk;
				walk_last = 0;
				continue;
			}
			segment = walk;
			if (preg_get_backref(&walk, &backref)) {
				if (backref < count) {
					/* Find the corresponding string match and substitute it
					   in instead of the backref */
					match = subject + offsets[backref<<1];
					match_len = offsets[(backref<<1)+1] - offsets[backref<<1];
					if (match_len) {
						esc_match = php_addslashes(match, match_len, &esc_match_len, 0 TSRMLS_CC);
					} else {
						esc_match = match;
						esc_match_len = 0;
					}
				} else {
					esc_match = "";
					esc_match_len = 0;
				}
				smart_str_appendl(&code, esc_match, esc_match_len);

				segment = walk;

				/* Clean up and reassign */
				if (esc_match_len)
					efree(esc_match);
				continue;
			}
		}
		walk++;
		walk_last = walk[-1];
	}
	smart_str_appendl(&code, segment, walk - segment);
	smart_str_0(&code);

	compiled_string_description = zend_make_compiled_string_description("regexp code" TSRMLS_CC);
	/* Run the code */
	if (zend_eval_stringl(code.c, code.len, &retval, compiled_string_description TSRMLS_CC) == FAILURE) {
		efree(compiled_string_description);
		php_error_docref(NULL TSRMLS_CC,E_ERROR, "Failed evaluating code: %s%s", PHP_EOL, code.c);
		/* zend_error() does not return in this case */
	}
	efree(compiled_string_description);
	convert_to_string(&retval);
	
	/* Save the return value and its length */
	*result = estrndup(Z_STRVAL(retval), Z_STRLEN(retval));
	result_len = Z_STRLEN(retval);
	
	/* Clean up */
	zval_dtor(&retval);
	smart_str_free(&code);
	
	return result_len;
}