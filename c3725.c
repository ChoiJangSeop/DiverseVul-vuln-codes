PHP_FUNCTION(bcmod)
{
	char *left, *right;
	int left_len, right_len;
	bc_num first, second, result;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss", &left, &left_len, &right, &right_len) == FAILURE) {
		return;
	}
	
	bc_init_num(&first TSRMLS_CC);
	bc_init_num(&second TSRMLS_CC);
	bc_init_num(&result TSRMLS_CC);
	bc_str2num(&first, left, 0 TSRMLS_CC);
	bc_str2num(&second, right, 0 TSRMLS_CC);
	
	switch (bc_modulo(first, second, &result, 0 TSRMLS_CC)) {
		case 0:
			Z_STRVAL_P(return_value) = bc_num2str(result);
			Z_STRLEN_P(return_value) = strlen(Z_STRVAL_P(return_value));
			Z_TYPE_P(return_value) = IS_STRING;
			break;
		case -1:
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Division by zero");
			break;
	}
	
	bc_free_num(&first);
	bc_free_num(&second);
	bc_free_num(&result);
	return;
}