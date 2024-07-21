PHP_FUNCTION(round)
{
	zval **value;
	int places = 0;
	long precision = 0;
	long mode = PHP_ROUND_HALF_UP;
	double return_val;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Z|ll", &value, &precision, &mode) == FAILURE) {
		return;
	}

	if (ZEND_NUM_ARGS() >= 2) {
		places = (int) precision;
	}
	convert_scalar_to_number_ex(value);

	switch (Z_TYPE_PP(value)) {
		case IS_LONG:
			/* Simple case - long that doesn't need to be rounded. */
			if (places >= 0) {
				RETURN_DOUBLE((double) Z_LVAL_PP(value));
			}
			/* break omitted intentionally */

		case IS_DOUBLE:
			return_val = (Z_TYPE_PP(value) == IS_LONG) ? (double)Z_LVAL_PP(value) : Z_DVAL_PP(value);
			return_val = _php_math_round(return_val, places, mode);
			RETURN_DOUBLE(return_val);
			break;

		default:
			RETURN_FALSE;
			break;
	}
}