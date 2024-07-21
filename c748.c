static int proc_parse_options(char *options, struct pid_namespace *pid)
{
	char *p;
	substring_t args[MAX_OPT_ARGS];

	pr_debug("proc: options = %s\n", options);

	if (!options)
		return 1;

	while ((p = strsep(&options, ",")) != NULL) {
		int token;
		if (!*p)
			continue;

		args[0].to = args[0].from = 0;
		token = match_token(p, tokens, args);
		switch (token) {
		default:
			pr_err("proc: unrecognized mount option \"%s\" "
			       "or missing value\n", p);
			return 0;
		}
	}

	return 1;
}