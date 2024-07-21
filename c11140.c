static char *shell_name_generator(const char *text, int state)
{
	static size_t len;
	char *s;

	if (!state) {
		setusershell();
		len = strlen(text);
	}

	while ((s = getusershell())) {
		if (strncmp(s, text, len) == 0)
			return xstrdup(s);
	}
	return NULL;
}