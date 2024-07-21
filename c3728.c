PHP_FUNCTION(bcscale)
{
	long new_scale;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &new_scale) == FAILURE) {
		return;
	}

	BCG(bc_precision) = ((int)new_scale < 0) ? 0 : new_scale;

	RETURN_TRUE;
}