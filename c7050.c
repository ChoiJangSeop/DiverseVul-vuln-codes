rdp_in_unistr(STREAM s, int in_len, char **string, uint32 * str_size)
{
	/* Dynamic allocate of destination string if not provided */
	*string = xmalloc(in_len * 2);
	*str_size = in_len * 2;

#ifdef HAVE_ICONV
	size_t ibl = in_len, obl = *str_size - 1;
	char *pin = (char *) s->p, *pout = *string;
	static iconv_t iconv_h = (iconv_t) - 1;

	if (g_iconv_works)
	{
		if (iconv_h == (iconv_t) - 1)
		{
			if ((iconv_h = iconv_open(g_codepage, WINDOWS_CODEPAGE)) == (iconv_t) - 1)
			{
				warning("rdp_in_unistr: iconv_open[%s -> %s] fail %p\n",
					WINDOWS_CODEPAGE, g_codepage, iconv_h);

				g_iconv_works = False;
				return rdp_in_unistr(s, in_len, string, str_size);
			}
		}

		if (iconv(iconv_h, (ICONV_CONST char **) &pin, &ibl, &pout, &obl) == (size_t) - 1)
		{
			if (errno == E2BIG)
			{
				warning("server sent an unexpectedly long string, truncating\n");
			}
			else
			{
				warning("rdp_in_unistr: iconv fail, errno %d\n", errno);

				free(*string);
				*string = NULL;
				*str_size = 0;
			}
		}

		/* we must update the location of the current STREAM for future reads of s->p */
		s->p += in_len;

		*pout = 0;

		if (*string)
			*str_size = pout - *string;
	}
	else
#endif
	{
		int i = 0;
		int rem = 0;
		uint32 len = in_len / 2;

		if (len > *str_size - 1)
		{
			warning("server sent an unexpectedly long string, truncating\n");
			len = *str_size - 1;
			rem = in_len - 2 * len;
		}

		while (i < len)
		{
			in_uint8a(s, &string[i++], 1);
			in_uint8s(s, 1);
		}

		in_uint8s(s, rem);
		string[len] = 0;
		*str_size = len;
	}
}