static void print_cpu(int pid) {
	char *file;
	if (asprintf(&file, "/proc/%d/status", pid) == -1) {
		errExit("asprintf");
		exit(1);
	}

	EUID_ROOT();	// grsecurity
	FILE *fp = fopen(file, "re");
	EUID_USER();	// grsecurity
	if (!fp) {
		printf("  Error: cannot open %s\n", file);
		free(file);
		return;
	}

#define MAXBUF 4096
	char buf[MAXBUF];
	while (fgets(buf, MAXBUF, fp)) {
		if (strncmp(buf, "Cpus_allowed_list:", 18) == 0) {
			printf("  %s", buf);
			fflush(0);
			free(file);
			fclose(fp);
			return;
		}
	}
	fclose(fp);
	free(file);
}