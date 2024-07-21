PHP_FUNCTION(bcpowmod)
{
	char *left, *right, *modulous;
	int left_len, right_len, modulous_len;
	bc_num first, second, mod, result;
	long scale = BCG(bc_precision);
	int scale_int;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sss|l", &left, &left_len, &right, &right_len, &modulous, &modulous_len, &scale) == FAILURE) {
		return;
	}

	bc_init_num(&first TSRMLS_CC);
	bc_init_num(&second TSRMLS_CC);
	bc_init_num(&mod TSRMLS_CC);
	bc_init_num(&result TSRMLS_CC);
	php_str2num(&first, left TSRMLS_CC);
	php_str2num(&second, right TSRMLS_CC);
	php_str2num(&mod, modulous TSRMLS_CC);

	scale_int = (int) ((int)scale < 0) ? 0 : scale;

	if (bc_raisemod(first, second, mod, &result, scale_int TSRMLS_CC) != -1) {
		if (result->n_scale > scale) {
			result->n_scale = scale;
		}
		Z_STRVAL_P(return_value) = bc_num2str(result);
		Z_STRLEN_P(return_value) = strlen(Z_STRVAL_P(return_value));
		Z_TYPE_P(return_value) = IS_STRING;
	} else {
		RETVAL_FALSE;
	}
	
	bc_free_num(&first);
	bc_free_num(&second);
	bc_free_num(&mod);
	bc_free_num(&result);
	return;
}