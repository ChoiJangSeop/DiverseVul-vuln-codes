PHP_FUNCTION(pcntl_setpriority)
{
	long who = PRIO_PROCESS;
	long pid = getpid();
	long pri;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l|ll", &pri, &pid, &who) == FAILURE) {
		RETURN_FALSE;
	}

	if (setpriority(who, pid, pri)) {
		PCNTL_G(last_error) = errno;
		switch (errno) {
			case ESRCH:
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "Error %d: No process was located using the given parameters", errno);
				break;
			case EINVAL:
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "Error %d: Invalid identifier flag", errno);
				break;
			case EPERM:
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "Error %d: A process was located, but neither its effective nor real user ID matched the effective user ID of the caller", errno);
				break;
			case EACCES:
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "Error %d: Only a super user may attempt to increase the process priority", errno);
				break;
			default:
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unknown error %d has occurred", errno);
				break;
		}
		RETURN_FALSE;
	}
	
	RETURN_TRUE;
}