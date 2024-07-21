uid_t pid_get_uid(pid_t pid) {
	EUID_ASSERT();
	uid_t rv = 0;

	// open status file
	char *file;
	if (asprintf(&file, "/proc/%u/status", pid) == -1) {
		perror("asprintf");
		exit(1);
	}
	EUID_ROOT();				  // grsecurity fix
	FILE *fp = fopen(file, "re");
	if (!fp) {
		free(file);
		fprintf(stderr, "Error: cannot open /proc file\n");
		exit(1);
	}

	// extract uid
	static const int PIDS_BUFLEN = 1024;
	char buf[PIDS_BUFLEN];
	while (fgets(buf, PIDS_BUFLEN - 1, fp)) {
		if (strncmp(buf, "Uid:", 4) == 0) {
			char *ptr = buf + 4;
			while (*ptr != '\0' && (*ptr == ' ' || *ptr == '\t')) {
				ptr++;
			}
			if (*ptr == '\0') {
				fprintf(stderr, "Error: cannot read /proc file\n");
				exit(1);
			}

			rv = atoi(ptr);
			break;			  // break regardless!
		}
	}

	fclose(fp);
	free(file);
	EUID_USER();				  // grsecurity fix

	return rv;
}