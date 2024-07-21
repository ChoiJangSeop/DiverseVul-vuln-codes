static void extract_nogroups(pid_t pid) {
	char *fname;
	if (asprintf(&fname, "/proc/%d/root%s", pid, RUN_GROUPS_CFG) == -1)
		errExit("asprintf");

	struct stat s;
	if (stat(fname, &s) == -1) {
		free(fname);
		return;
	}

	arg_nogroups = 1;
	free(fname);
}