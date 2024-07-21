PHP_FUNCTION(pcntl_getpriority)
{
	long who = PRIO_PROCESS;
	long pid = getpid();
	int pri;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|ll", &pid, &who) == FAILURE) {
		RETURN_FALSE;
	}

	/* needs to be cleared, since any returned value is valid */ 
	errno = 0;

	pri = getpriority(who, pid);

	if (errno) {
		PCNTL_G(last_error) = errno;
		switch (errno) {
			case ESRCH:
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "Error %d: No process was located using the given parameters", errno);
				break;
			case EINVAL:
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "Error %d: Invalid identifier flag", errno);
				break;
			default:
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unknown error %d has occurred", errno);
				break;
		}
		RETURN_FALSE;
	}

	RETURN_LONG(pri);
}