char *make_argv(char *buf, size_t len, int argc, char **argv)
{
	size_t left= len;
	const char *arg;

	buf[0]= 0;
	++argv; --argc;
	while (argc > 0 && left > 0)
	{
		arg = *argv;
		if (strncmp(*argv, "--password", strlen("--password")) == 0) {
			arg = "--password=...";
		}
		if (strncmp(*argv, "--encrypt-key",
				strlen("--encrypt-key")) == 0) {
			arg = "--encrypt-key=...";
		}
		if (strncmp(*argv, "--encrypt_key",
				strlen("--encrypt_key")) == 0) {
			arg = "--encrypt_key=...";
		}
		if (strncmp(*argv, "--transition-key",
				strlen("--transition-key")) == 0) {
			arg = "--transition-key=...";
		}
		if (strncmp(*argv, "--transition_key",
				strlen("--transition_key")) == 0) {
			arg = "--transition_key=...";
		}
		left-= ut_snprintf(buf + len - left, left,
			"%s%c", arg, argc > 1 ? ' ' : 0);
		++argv; --argc;
	}

	return buf;
}