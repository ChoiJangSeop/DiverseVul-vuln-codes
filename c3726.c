PHP_FUNCTION(bccomp)
{
	char *left, *right;
	int left_len, right_len;
	long scale_param = 0;
	bc_num first, second;
	int scale = BCG(bc_precision), argc = ZEND_NUM_ARGS();

	if (zend_parse_parameters(argc TSRMLS_CC, "ss|l", &left, &left_len, &right, &right_len, &scale_param) == FAILURE) {
		return;
	}
	
	if (argc == 3) {
		scale = (int) ((int)scale_param < 0) ? 0 : scale_param;
	}

	bc_init_num(&first TSRMLS_CC);
	bc_init_num(&second TSRMLS_CC);

	bc_str2num(&first, left, scale TSRMLS_CC);
	bc_str2num(&second, right, scale TSRMLS_CC);
	Z_LVAL_P(return_value) = bc_compare(first, second);
	Z_TYPE_P(return_value) = IS_LONG;

	bc_free_num(&first);
	bc_free_num(&second);
	return;
}