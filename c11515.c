static void extract_caps(pid_t pid) {
	// open stat file
	char *file;
	if (asprintf(&file, "/proc/%u/status", pid) == -1) {
		perror("asprintf");
		exit(1);
	}
	FILE *fp = fopen(file, "re");
	if (!fp)
		goto errexit;

	char buf[BUFLEN];
	while (fgets(buf, BUFLEN - 1, fp)) {
		if (strncmp(buf, "CapBnd:", 7) == 0) {
			char *ptr = buf + 7;
			unsigned long long val;
			if (sscanf(ptr, "%llx", &val) != 1)
				goto errexit;
			apply_caps = 1;
			caps = val;
		}
		else if (strncmp(buf, "NoNewPrivs:", 11) == 0) {
			char *ptr = buf + 11;
			int val;
			if (sscanf(ptr, "%d", &val) != 1)
				goto errexit;
			if (val)
				arg_nonewprivs = 1;
		}
	}
	fclose(fp);
	free(file);
	return;

errexit:
	fprintf(stderr, "Error: cannot read stat file for process %u\n", pid);
	exit(1);
}