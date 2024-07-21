static void forward_filesfrom_data(void)
{
	ssize_t len;

	len = read(ff_forward_fd, ff_xb.buf + ff_xb.len, ff_xb.size - ff_xb.len);
	if (len <= 0) {
		if (len == 0 || errno != EINTR) {
			/* Send end-of-file marker */
			ff_forward_fd = -1;
			write_buf(iobuf.out_fd, "\0\0", ff_lastchar ? 2 : 1);
			free_xbuf(&ff_xb);
			if (ff_reenable_multiplex >= 0)
				io_start_multiplex_out(ff_reenable_multiplex);
		}
		return;
	}

	if (DEBUG_GTE(IO, 2)) {
		rprintf(FINFO, "[%s] files-from read=%" SIZE_T_FMT_MOD "d\n",
			who_am_i(), (SIZE_T_FMT_CAST)len);
	}

#ifdef ICONV_OPTION
	len += ff_xb.len;
#endif

	if (!eol_nulls) {
		char *s = ff_xb.buf + len;
		/* Transform CR and/or LF into '\0' */
		while (s-- > ff_xb.buf) {
			if (*s == '\n' || *s == '\r')
				*s = '\0';
		}
	}

	if (ff_lastchar)
		ff_xb.pos = 0;
	else {
		char *s = ff_xb.buf;
		/* Last buf ended with a '\0', so don't let this buf start with one. */
		while (len && *s == '\0')
			s++, len--;
		ff_xb.pos = s - ff_xb.buf;
	}

#ifdef ICONV_OPTION
	if (filesfrom_convert && len) {
		char *sob = ff_xb.buf + ff_xb.pos, *s = sob;
		char *eob = sob + len;
		int flags = ICB_INCLUDE_BAD | ICB_INCLUDE_INCOMPLETE | ICB_CIRCULAR_OUT;
		if (ff_lastchar == '\0')
			flags |= ICB_INIT;
		/* Convert/send each null-terminated string separately, skipping empties. */
		while (s != eob) {
			if (*s++ == '\0') {
				ff_xb.len = s - sob - 1;
				if (iconvbufs(ic_send, &ff_xb, &iobuf.out, flags) < 0)
					exit_cleanup(RERR_PROTOCOL); /* impossible? */
				write_buf(iobuf.out_fd, s-1, 1); /* Send the '\0'. */
				while (s != eob && *s == '\0')
					s++;
				sob = s;
				ff_xb.pos = sob - ff_xb.buf;
				flags |= ICB_INIT;
			}
		}

		if ((ff_xb.len = s - sob) == 0)
			ff_lastchar = '\0';
		else {
			/* Handle a partial string specially, saving any incomplete chars. */
			flags &= ~ICB_INCLUDE_INCOMPLETE;
			if (iconvbufs(ic_send, &ff_xb, &iobuf.out, flags) < 0) {
				if (errno == E2BIG)
					exit_cleanup(RERR_PROTOCOL); /* impossible? */
				if (ff_xb.pos)
					memmove(ff_xb.buf, ff_xb.buf + ff_xb.pos, ff_xb.len);
			}
			ff_lastchar = 'x'; /* Anything non-zero. */
		}
	} else
#endif

	if (len) {
		char *f = ff_xb.buf + ff_xb.pos;
		char *t = ff_xb.buf;
		char *eob = f + len;
		/* Eliminate any multi-'\0' runs. */
		while (f != eob) {
			if (!(*t++ = *f++)) {
				while (f != eob && *f == '\0')
					f++;
			}
		}
		ff_lastchar = f[-1];
		if ((len = t - ff_xb.buf) != 0) {
			/* This will not circle back to perform_io() because we only get
			 * called when there is plenty of room in the output buffer. */
			write_buf(iobuf.out_fd, ff_xb.buf, len);
		}
	}
}