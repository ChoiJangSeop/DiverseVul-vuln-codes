PHP_FUNCTION(bcadd)
{
	char *left, *right;
	long scale_param = 0;
	bc_num first, second, result;
	int left_len, right_len;
	int scale = BCG(bc_precision), argc = ZEND_NUM_ARGS();

	if (zend_parse_parameters(argc TSRMLS_CC, "ss|l", &left, &left_len, &right, &right_len, &scale_param) == FAILURE) {
		return;
	}
	
	if (argc == 3) {
		scale = (int) ((int)scale_param < 0) ? 0 : scale_param;
	}

	bc_init_num(&first TSRMLS_CC);
	bc_init_num(&second TSRMLS_CC);
	bc_init_num(&result TSRMLS_CC);
	php_str2num(&first, left TSRMLS_CC);
	php_str2num(&second, right TSRMLS_CC);
	bc_add (first, second, &result, scale);
	
	if (result->n_scale > scale) {
		result->n_scale = scale;
	}
	
	Z_STRVAL_P(return_value) = bc_num2str(result);
	Z_STRLEN_P(return_value) = strlen(Z_STRVAL_P(return_value));
	Z_TYPE_P(return_value) = IS_STRING;
	bc_free_num(&first);
	bc_free_num(&second);
	bc_free_num(&result);
	return;
}