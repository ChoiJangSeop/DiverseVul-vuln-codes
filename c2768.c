PHP_FUNCTION(umask)
{
	long arg1 = 0;
	int oldumask;
	
	oldumask = umask(077);

	if (BG(umask) == -1) {
		BG(umask) = oldumask;
	}
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|l", &arg1) == FAILURE) {
		RETURN_FALSE;
	}

	if (ZEND_NUM_ARGS() == 0) {
		umask(oldumask);
	} else {
		umask(arg1);
	}

	RETURN_LONG(oldumask);
}