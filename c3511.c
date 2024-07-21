PHP_FUNCTION(escapeshellcmd)
{
	char *command;
	size_t command_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "s", &command, &command_len) == FAILURE) {
		return;
	}

	if (command_len) {
		RETVAL_STR(php_escape_shell_cmd(command));
	} else {
		RETVAL_EMPTY_STRING();
	}
}