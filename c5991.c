char *theme_format_expand_get(THEME_REC *theme, const char **format)
{
	GString *str;
	char *ret, dummy;
	int braces = 1; /* we start with one brace opened */

	str = g_string_new(NULL);
	while (**format != '\0' && braces != 0) {
		if (**format == '{')
			braces++;
		else if (**format == '}')
			braces--;
		else {
			theme_format_append_next(theme, str, format,
						 'n', 'n',
						 &dummy, &dummy, 0);
			continue;
		}
		
		if (braces == 0) {
			(*format)++;
			break;
		}

		g_string_append_c(str, **format);
		(*format)++;
	}

	ret = str->str;
        g_string_free(str, FALSE);
        return ret;
}