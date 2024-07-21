static char *theme_format_expand_abstract(THEME_REC *theme,
					  const char **formatp,
					  char default_fg, char default_bg,
					  int flags)
{
	const char *p, *format;
	char *abstract, *data, *ret;
	int len;

	format = *formatp;

	/* get abstract name first */
	p = format;
	while (*p != '\0' && *p != ' ' &&
	       *p != '{' && *p != '}') p++;
	if (*p == '\0' || p == format)
		return NULL; /* error */

	len = (int) (p-format);
	abstract = g_strndup(format, len);

	/* skip the following space, if there's any more spaces they're
	   treated as arguments */
	if (*p == ' ') {
		len++;
		if ((flags & EXPAND_FLAG_IGNORE_EMPTY) && data_is_empty(&p)) {
			*formatp = p;
			g_free(abstract);
			return NULL;
		}
	}
	*formatp = format+len;

	/* get the abstract data */
	data = g_hash_table_lookup(theme->abstracts, abstract);
	g_free(abstract);
	if (data == NULL) {
		/* unknown abstract, just display the data */
		data = "$0-";
	}
	abstract = g_strdup(data);

	/* we'll need to get the data part. it may contain
	   more abstracts, they are _NOT_ expanded. */
	data = theme_format_expand_get(theme, formatp);
	len = strlen(data);

	if (len > 1 && i_isdigit(data[len-1]) && data[len-2] == '$') {
		/* ends with $<digit> .. this breaks things if next
		   character is digit or '-' */
                char digit, *tmp;

		tmp = data;
		digit = tmp[len-1];
		tmp[len-1] = '\0';

		data = g_strdup_printf("%s{%c}", tmp, digit);
		g_free(tmp);
	}

	ret = parse_special_string(abstract, NULL, NULL, data, NULL,
				   PARSE_FLAG_ONLY_ARGS);
	g_free(abstract);
        g_free(data);
	abstract = ret;

	/* abstract may itself contain abstracts or replaces */
	p = abstract;
	ret = theme_format_expand_data(theme, &p, default_fg, default_bg,
				       &default_fg, &default_bg,
				       flags | EXPAND_FLAG_LASTCOLOR_ARG);
	g_free(abstract);
	return ret;
}