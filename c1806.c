file_ascmagic_with_encoding(struct magic_set *ms, const unsigned char *buf,
    size_t nbytes, unichar *ubuf, size_t ulen, const char *code,
    const char *type, int text)
{
	unsigned char *utf8_buf = NULL, *utf8_end;
	size_t mlen, i;
	int rv = -1;
	int mime = ms->flags & MAGIC_MIME;

	const char *subtype = NULL;
	const char *subtype_mime = NULL;

	int has_escapes = 0;
	int has_backspace = 0;
	int seen_cr = 0;

	int n_crlf = 0;
	int n_lf = 0;
	int n_cr = 0;
	int n_nel = 0;
	int executable = 0;

	size_t last_line_end = (size_t)-1;
	int has_long_lines = 0;

	if (ms->flags & MAGIC_APPLE)
		return 0;

	nbytes = trim_nuls(buf, nbytes);

	/* If we have fewer than 2 bytes, give up. */
	if (nbytes <= 1) {
		rv = 0;
		goto done;
	}

	if (ulen > 0 && (ms->flags & MAGIC_NO_CHECK_SOFT) == 0) {
		/* Convert ubuf to UTF-8 and try text soft magic */
		/* malloc size is a conservative overestimate; could be
		   improved, or at least realloced after conversion. */
		mlen = ulen * 6;
		if ((utf8_buf = CAST(unsigned char *, emalloc(mlen))) == NULL) {
			file_oomem(ms, mlen);
			goto done;
		}
		if ((utf8_end = encode_utf8(utf8_buf, mlen, ubuf, ulen))
		    == NULL)
			goto done;
		if ((rv = file_softmagic(ms, utf8_buf,
		    (size_t)(utf8_end - utf8_buf), TEXTTEST, text)) == 0)
			rv = -1;
	}

	/* Now try to discover other details about the file. */
	for (i = 0; i < ulen; i++) {
		if (ubuf[i] == '\n') {
			if (seen_cr)
				n_crlf++;
			else
				n_lf++;
			last_line_end = i;
		} else if (seen_cr)
			n_cr++;

		seen_cr = (ubuf[i] == '\r');
		if (seen_cr)
			last_line_end = i;

		if (ubuf[i] == 0x85) { /* X3.64/ECMA-43 "next line" character */
			n_nel++;
			last_line_end = i;
		}

		/* If this line is _longer_ than MAXLINELEN, remember it. */
		if (i > last_line_end + MAXLINELEN)
			has_long_lines = 1;

		if (ubuf[i] == '\033')
			has_escapes = 1;
		if (ubuf[i] == '\b')
			has_backspace = 1;
	}

	/* Beware, if the data has been truncated, the final CR could have
	   been followed by a LF.  If we have HOWMANY bytes, it indicates
	   that the data might have been truncated, probably even before
	   this function was called. */
	if (seen_cr && nbytes < HOWMANY)
		n_cr++;

	if (strcmp(type, "binary") == 0) {
		rv = 0;
		goto done;
	}
	if (mime) {
		if (!file_printedlen(ms) && (mime & MAGIC_MIME_TYPE) != 0) {
			if (subtype_mime) {
				if (file_printf(ms, "%s", subtype_mime) == -1)
					goto done;
			} else {
				if (file_printf(ms, "text/plain") == -1)
					goto done;
			}
		}
	} else {
		if (file_printedlen(ms)) {
			switch (file_replace(ms, " text$", ", ")) {
			case 0:
				switch (file_replace(ms, " text executable$",
				    ", ")) {
				case 0:
					if (file_printf(ms, ", ") == -1)
						goto done;
					break;
				case -1:
					goto done;
				default:
					executable = 1;
					break;
				}
				break;
			case -1:
				goto done;
			default:
				break;
			}
		}

		if (file_printf(ms, "%s", code) == -1)
			goto done;

		if (subtype) {
			if (file_printf(ms, " %s", subtype) == -1)
				goto done;
		}

		if (file_printf(ms, " %s", type) == -1)
			goto done;

		if (executable)
			if (file_printf(ms, " executable") == -1)
				goto done;

		if (has_long_lines)
			if (file_printf(ms, ", with very long lines") == -1)
				goto done;

		/*
		 * Only report line terminators if we find one other than LF,
		 * or if we find none at all.
		 */
		if ((n_crlf == 0 && n_cr == 0 && n_nel == 0 && n_lf == 0) ||
		    (n_crlf != 0 || n_cr != 0 || n_nel != 0)) {
			if (file_printf(ms, ", with") == -1)
				goto done;

			if (n_crlf == 0 && n_cr == 0 && n_nel == 0 && n_lf == 0) {
				if (file_printf(ms, " no") == -1)
					goto done;
			} else {
				if (n_crlf) {
					if (file_printf(ms, " CRLF") == -1)
						goto done;
					if (n_cr || n_lf || n_nel)
						if (file_printf(ms, ",") == -1)
							goto done;
				}
				if (n_cr) {
					if (file_printf(ms, " CR") == -1)
						goto done;
					if (n_lf || n_nel)
						if (file_printf(ms, ",") == -1)
							goto done;
				}
				if (n_lf) {
					if (file_printf(ms, " LF") == -1)
						goto done;
					if (n_nel)
						if (file_printf(ms, ",") == -1)
							goto done;
				}
				if (n_nel)
					if (file_printf(ms, " NEL") == -1)
						goto done;
			}

			if (file_printf(ms, " line terminators") == -1)
				goto done;
		}

		if (has_escapes)
			if (file_printf(ms, ", with escape sequences") == -1)
				goto done;
		if (has_backspace)
			if (file_printf(ms, ", with overstriking") == -1)
				goto done;
	}
	rv = 1;
done:
	if (utf8_buf)
		efree(utf8_buf);

	return rv;
}