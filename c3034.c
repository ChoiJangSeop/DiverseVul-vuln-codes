static PHP_FUNCTION(preg_quote)
{
	int		 in_str_len;
	char	*in_str;		/* Input string argument */
	char	*in_str_end;    /* End of the input string */
	int		 delim_len = 0;
	char	*delim = NULL;	/* Additional delimiter argument */
	char	*out_str,		/* Output string with quoted characters */
		 	*p,				/* Iterator for input string */
			*q,				/* Iterator for output string */
			 delim_char=0,	/* Delimiter character to be quoted */
			 c;				/* Current character */
	zend_bool quote_delim = 0; /* Whether to quote additional delim char */
	
	/* Get the arguments and check for errors */
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|s", &in_str, &in_str_len,
							  &delim, &delim_len) == FAILURE) {
		return;
	}
	
	in_str_end = in_str + in_str_len;

	/* Nothing to do if we got an empty string */
	if (in_str == in_str_end) {
		RETURN_EMPTY_STRING();
	}

	if (delim && *delim) {
		delim_char = delim[0];
		quote_delim = 1;
	}
	
	/* Allocate enough memory so that even if each character
	   is quoted, we won't run out of room */
	out_str = safe_emalloc(4, in_str_len, 1);
	
	/* Go through the string and quote necessary characters */
	for(p = in_str, q = out_str; p != in_str_end; p++) {
		c = *p;
		switch(c) {
			case '.':
			case '\\':
			case '+':
			case '*':
			case '?':
			case '[':
			case '^':
			case ']':
			case '$':
			case '(':
			case ')':
			case '{':
			case '}':
			case '=':
			case '!':
			case '>':
			case '<':
			case '|':
			case ':':
			case '-':
				*q++ = '\\';
				*q++ = c;
				break;

			case '\0':
				*q++ = '\\';
				*q++ = '0';
				*q++ = '0';
				*q++ = '0';
				break;

			default:
				if (quote_delim && c == delim_char)
					*q++ = '\\';
				*q++ = c;
				break;
		}
	}
	*q = '\0';
	
	/* Reallocate string and return it */
	RETVAL_STRINGL(erealloc(out_str, q - out_str + 1), q - out_str, 0);
}