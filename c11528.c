static uint64_t extract_caps(int pid) {
	EUID_ASSERT();

	char *file;
	if (asprintf(&file, "/proc/%d/status", pid) == -1)
		errExit("asprintf");

	EUID_ROOT();	// grsecurity
	FILE *fp = fopen(file, "re");
	EUID_USER();	// grsecurity
	if (!fp)
		goto errexit;

	char buf[MAXBUF];
	while (fgets(buf, MAXBUF, fp)) {
		if (strncmp(buf, "CapBnd:\t", 8) == 0) {
			char *ptr = buf + 8;
			unsigned long long val;
			sscanf(ptr, "%llx", &val);
			free(file);
			fclose(fp);
			return val;
		}
	}
	fclose(fp);

errexit:
	free(file);
	fprintf(stderr, "Error: cannot read caps configuration\n");
	exit(1);
}