static char *ask_new_field(struct chfn_control *ctl, const char *question,
			   char *def_val)
{
	int len;
	char *buf = NULL; /* leave initialized to NULL or getline segfaults */
#ifndef HAVE_LIBREADLINE
	size_t dummy = 0;
#endif

	if (!def_val)
		def_val = "";
	while (true) {
		printf("%s [%s]:", question, def_val);
		__fpurge(stdin);
#ifdef HAVE_LIBREADLINE
		rl_bind_key('\t', rl_insert);
		if ((buf = readline(" ")) == NULL)
#else
		putchar(' ');
		fflush(stdout);
		if (getline(&buf, &dummy, stdin) < 0)
#endif
			errx(EXIT_FAILURE, _("Aborted."));
		/* remove white spaces from string end */
		ltrim_whitespace((unsigned char *) buf);
		len = rtrim_whitespace((unsigned char *) buf);
		if (len == 0) {
			free(buf);
			return xstrdup(def_val);
		}
		if (!strcasecmp(buf, "none")) {
			free(buf);
			ctl->changed = 1;
			return xstrdup("");
		}
		if (check_gecos_string(question, buf) >= 0)
			break;
	}
	ctl->changed = 1;
	return buf;
}