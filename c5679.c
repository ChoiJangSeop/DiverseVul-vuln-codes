char **recode_split(const SERVER_REC *server, const char *str,
		    const char *target, int len, gboolean onspace)
{
	GIConv cd = (GIConv)-1;
	const char *from = translit_charset;
	const char *to = translit_charset;
	char *translit_to = NULL;
	const char *inbuf = str;
	const char *previnbuf = inbuf;
	char *tmp = NULL;
	char *outbuf;
	gsize inbytesleft = strlen(inbuf);
	gsize outbytesleft = len;
	int n = 0;
	char **ret;

	g_return_val_if_fail(str != NULL, NULL);

	if (settings_get_bool("recode")) {
		to = find_conversion(server, target);
		if (to == NULL)
			/* default outgoing charset if set */
			to = settings_get_str("recode_out_default_charset");
		if (to != NULL && *to != '\0') {
			if (settings_get_bool("recode_transliterate") &&
			    !is_translit(to))
				to = translit_to = g_strconcat(to,
							       "//TRANSLIT",
							       NULL);
		} else {
			to = from;
		}
	}

	cd = g_iconv_open(to, from);
	if (cd == (GIConv)-1) {
		/* Fall back to splitting by byte. */
		ret = strsplit_len(str, len, onspace);
		goto out;
	}

	tmp = g_malloc(outbytesleft);
	outbuf = tmp;
	ret = g_new(char *, 1);
	while (g_iconv(cd, (char **)&inbuf, &inbytesleft, &outbuf,
		       &outbytesleft) == -1) {
		if (errno != E2BIG) {
			/*
			 * Conversion failed. Fall back to splitting
			 * by byte.
			 */
			ret[n] = NULL;
			g_strfreev(ret);
			ret = strsplit_len(str, len, onspace);
			goto out;
		}

		/* Outbuf overflowed, split the input string. */
		if (onspace) {
			/*
			 * Try to find a space to split on and leave
			 * the space on the previous line.
			 */
			int i;
			for (i = 0; i < inbuf - previnbuf; i++) {
				if (previnbuf[inbuf-previnbuf-1-i] == ' ') {
					inbuf -= i;
					inbytesleft += i;
					break;
				}
			}
		}
		ret[n++] = g_strndup(previnbuf, inbuf - previnbuf);
		ret = g_renew(char *, ret, n + 1);
		previnbuf = inbuf;

		/* Reset outbuf for the next substring. */
		outbuf = tmp;
		outbytesleft = len;
	}
	/* Copy the last substring into the array. */
	ret[n++] = g_strndup(previnbuf, inbuf - previnbuf);
	ret = g_renew(char *, ret, n + 1);
	ret[n] = NULL;

out:
	if (cd != (GIConv)-1)
		g_iconv_close(cd);
	g_free(translit_to);
	g_free(tmp);

	return ret;
}