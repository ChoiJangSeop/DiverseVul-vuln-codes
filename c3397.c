PHPAPI zend_string *php_escape_shell_cmd(char *str)
{
	register int x, y, l = (int)strlen(str);
	size_t estimate = (2 * l) + 1;
	zend_string *cmd;
#ifndef PHP_WIN32
	char *p = NULL;
#endif


	cmd = zend_string_alloc(2 * l, 0);

	for (x = 0, y = 0; x < l; x++) {
		int mb_len = php_mblen(str + x, (l - x));

		/* skip non-valid multibyte characters */
		if (mb_len < 0) {
			continue;
		} else if (mb_len > 1) {
			memcpy(ZSTR_VAL(cmd) + y, str + x, mb_len);
			y += mb_len;
			x += mb_len - 1;
			continue;
		}

		switch (str[x]) {
#ifndef PHP_WIN32
			case '"':
			case '\'':
				if (!p && (p = memchr(str + x + 1, str[x], l - x - 1))) {
					/* noop */
				} else if (p && *p == str[x]) {
					p = NULL;
				} else {
					ZSTR_VAL(cmd)[y++] = '\\';
				}
				ZSTR_VAL(cmd)[y++] = str[x];
				break;
#else
			/* % is Windows specific for environmental variables, ^%PATH% will 
				output PATH while ^%PATH^% will not. escapeshellcmd->val will escape all % and !.
			*/
			case '%':
			case '!':
			case '"':
			case '\'':
#endif
			case '#': /* This is character-set independent */
			case '&':
			case ';':
			case '`':
			case '|':
			case '*':
			case '?':
			case '~':
			case '<':
			case '>':
			case '^':
			case '(':
			case ')':
			case '[':
			case ']':
			case '{':
			case '}':
			case '$':
			case '\\':
			case '\x0A': /* excluding these two */
			case '\xFF':
#ifdef PHP_WIN32
				ZSTR_VAL(cmd)[y++] = '^';
#else
				ZSTR_VAL(cmd)[y++] = '\\';
#endif
				/* fall-through */
			default:
				ZSTR_VAL(cmd)[y++] = str[x];

		}
	}
	ZSTR_VAL(cmd)[y] = '\0';

	if ((estimate - y) > 4096) {
		/* realloc if the estimate was way overill
		 * Arbitrary cutoff point of 4096 */
		cmd = zend_string_truncate(cmd, y, 0);
	}

	ZSTR_LEN(cmd) = y;

	return cmd;
}