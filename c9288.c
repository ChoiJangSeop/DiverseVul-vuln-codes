get_password(const char *prompt, char *input, int capacity)
{
#ifdef ENABLE_SYSTEMD
	int is_systemd_running;
	struct stat a, b;

	/* We simply test whether the systemd cgroup hierarchy is
	 * mounted */
	is_systemd_running = (lstat("/sys/fs/cgroup", &a) == 0)
		&& (lstat("/sys/fs/cgroup/systemd", &b) == 0)
		&& (a.st_dev != b.st_dev);

	if (is_systemd_running) {
		char *cmd, *ret;
		FILE *ask_pass_fp = NULL;

		cmd = ret = NULL;
		if (asprintf(&cmd, "systemd-ask-password \"%s\"", prompt) >= 0) {
			ask_pass_fp = popen (cmd, "re");
			free (cmd);
		}

		if (ask_pass_fp) {
			ret = fgets(input, capacity, ask_pass_fp);
			pclose(ask_pass_fp);
		}

		if (ret) {
			int len = strlen(input);
			if (input[len - 1] == '\n')
				input[len - 1] = '\0';
			return input;
		}
	}
#endif

	/*
	 * Falling back to getpass(..)
	 * getpass is obsolete, but there's apparently nothing that replaces it
	 */
	char *tmp_pass = getpass(prompt);
	if (!tmp_pass)
		return NULL;

	strncpy(input, tmp_pass, capacity - 1);
	input[capacity - 1] = '\0';

	/* zero-out the static buffer */
	memset(tmp_pass, 0, strlen(tmp_pass));

	return input;
}