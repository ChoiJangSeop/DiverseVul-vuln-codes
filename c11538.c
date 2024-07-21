static void extract_cpu(pid_t pid) {
	char *fname;
	if (asprintf(&fname, "/proc/%d/root%s", pid, RUN_CPU_CFG) == -1)
		errExit("asprintf");

	struct stat s;
	if (stat(fname, &s) == -1) {
		free(fname);
		return;
	}

	// there is a CPU_CFG file, load it!
	load_cpu(fname);
	free(fname);
}