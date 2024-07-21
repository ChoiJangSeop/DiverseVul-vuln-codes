PHP_FUNCTION(pcntl_waitpid)
{
	long pid, options = 0;
	zval *z_status = NULL;
	int status;
	pid_t child_id;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lz|l", &pid, &z_status, &options) == FAILURE)
		return;
	
	convert_to_long_ex(&z_status);

	status = Z_LVAL_P(z_status);

	child_id = waitpid((pid_t) pid, &status, options);

	if (child_id < 0) {
		PCNTL_G(last_error) = errno;
	}

	Z_LVAL_P(z_status) = status;

	RETURN_LONG((long) child_id);
}