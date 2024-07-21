static int preg_do_repl_func(zval *function, char *subject, int *offsets, char **subpat_names, int count, char **result TSRMLS_DC)
{
	zval		*retval_ptr;		/* Function return value */
	zval	   **args[1];			/* Argument to pass to function */
	zval		*subpats;			/* Captured subpatterns */ 
	int			 result_len;		/* Return value length */
	int			 i;

	MAKE_STD_ZVAL(subpats);
	array_init(subpats);
	for (i = 0; i < count; i++) {
		if (subpat_names[i]) {
			add_assoc_stringl(subpats, subpat_names[i], &subject[offsets[i<<1]] , offsets[(i<<1)+1] - offsets[i<<1], 1);
		}
		add_next_index_stringl(subpats, &subject[offsets[i<<1]], offsets[(i<<1)+1] - offsets[i<<1], 1);
	}
	args[0] = &subpats;

	if (call_user_function_ex(EG(function_table), NULL, function, &retval_ptr, 1, args, 0, NULL TSRMLS_CC) == SUCCESS && retval_ptr) {
		convert_to_string_ex(&retval_ptr);
		*result = estrndup(Z_STRVAL_P(retval_ptr), Z_STRLEN_P(retval_ptr));
		result_len = Z_STRLEN_P(retval_ptr);
		zval_ptr_dtor(&retval_ptr);
	} else {
		if (!EG(exception)) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to call custom replacement function");
		}
		result_len = offsets[1] - offsets[0];
		*result = estrndup(&subject[offsets[0]], result_len);
	}

	zval_ptr_dtor(&subpats);

	return result_len;
}