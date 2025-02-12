escape_xml(const char *text)
{
	static char *escaped;
	static size_t escaped_size;
	char *out;
	size_t len;

	for (out=escaped, len=0; *text; ++len, ++out, ++text) {
		/* Make sure there's plenty of room for a quoted character */
		if ((len + 8) > escaped_size) {
			char *bigger_escaped;
			escaped_size += 128;
			bigger_escaped = realloc(escaped, escaped_size);
			if (!bigger_escaped) {
				free(escaped);	/* avoid leaking memory */
				escaped = NULL;
				escaped_size = 0;
				/* Error string is cleverly chosen to fail XML validation */
				return ">>> out of memory <<<";
			}
			out = bigger_escaped + len;
			escaped = bigger_escaped;
		}
		switch (*text) {
			case '&':
				strcpy(out, "&amp;");
				len += strlen(out) - 1;
				out = escaped + len;
				break;
			case '<':
				strcpy(out, "&lt;");
				len += strlen(out) - 1;
				out = escaped + len;
				break;
			case '>':
				strcpy(out, "&gt;");
				len += strlen(out) - 1;
				out = escaped + len;
				break;
			default:
				*out = *text;
				break;
		}
	}
	*out = '\x0';  /* NUL terminate the string */
	return escaped;
}