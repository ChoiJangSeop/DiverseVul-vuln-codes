PHP_FUNCTION(pcntl_fork)
{
	pid_t id;

	id = fork();
	if (id == -1) {
		PCNTL_G(last_error) = errno;
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Error %d", errno);
	}
	
	RETURN_LONG((long) id);
}