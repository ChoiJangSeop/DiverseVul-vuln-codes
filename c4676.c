int term_addstr(TERM_WINDOW *window, const char *str)
{
	int len, raw_len;
	unichar tmp;
	const char *ptr;

	if (vcmove) term_move_real();

	len = 0;
	raw_len = strlen(str);

	/* The string length depends on the terminal encoding */

	ptr = str;

	if (term_type == TERM_TYPE_UTF8) {
		while (*ptr != '\0') {
			tmp = g_utf8_get_char(ptr);
			len += unichar_isprint(tmp) ? mk_wcwidth(tmp) : 1;
			ptr = g_utf8_next_char(ptr);
		}
	} else
		len = raw_len;

        term_printed_text(len);

	/* Use strlen() here since we need the number of raw bytes */
	fwrite(str, 1, raw_len, window->term->out);

	return len;
}