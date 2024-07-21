PHP_FUNCTION(pcntl_wait)
{
	long options = 0;
	zval *z_status = NULL;
	int status;
	pid_t child_id;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z|l", &z_status, &options) == FAILURE)
		return;
	
	convert_to_long_ex(&z_status);

	status = Z_LVAL_P(z_status);
#ifdef HAVE_WAIT3
	if(options) {
		child_id = wait3(&status, options, NULL);
	}
	else {
		child_id = wait(&status);
	}
#else
	child_id = wait(&status);
#endif
	if (child_id < 0) {
		PCNTL_G(last_error) = errno;
	}

	Z_LVAL_P(z_status) = status;

	RETURN_LONG((long) child_id);
}