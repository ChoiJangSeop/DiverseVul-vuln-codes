PHP_FUNCTION(bcsqrt)
{
	char *left;
	int left_len;
	long scale_param = 0;
	bc_num result;
	int scale = BCG(bc_precision), argc = ZEND_NUM_ARGS();

	if (zend_parse_parameters(argc TSRMLS_CC, "s|l", &left, &left_len, &scale_param) == FAILURE) {
		return;
	}
	
	if (argc == 2) {
		scale = (int) ((int)scale_param < 0) ? 0 : scale_param;
	}

	bc_init_num(&result TSRMLS_CC);
	php_str2num(&result, left TSRMLS_CC);
	
	if (bc_sqrt (&result, scale TSRMLS_CC) != 0) {
		if (result->n_scale > scale) {
			result->n_scale = scale;
		}
		Z_STRVAL_P(return_value) = bc_num2str(result);
		Z_STRLEN_P(return_value) = strlen(Z_STRVAL_P(return_value));
		Z_TYPE_P(return_value) = IS_STRING;
	} else {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Square root of negative number");
	}

	bc_free_num(&result);
	return;
}