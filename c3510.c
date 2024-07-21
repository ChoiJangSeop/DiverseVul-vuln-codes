PHP_FUNCTION(escapeshellarg)
{
	char *argument;
	size_t argument_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "s", &argument, &argument_len) == FAILURE) {
		return;
	}

	if (argument) {
		RETVAL_STR(php_escape_shell_arg(argument));
	}
}