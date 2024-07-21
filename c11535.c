static void extract_umask(pid_t pid) {
	char *fname;
	if (asprintf(&fname, "/proc/%d/root%s", pid, RUN_UMASK_FILE) == -1)
		errExit("asprintf");

	FILE *fp = fopen(fname, "re");
	free(fname);
	if (!fp) {
		fprintf(stderr, "Error: cannot open umask file\n");
		exit(1);
	}
	if (fscanf(fp, "%3o", &orig_umask) != 1) {
		fprintf(stderr, "Error: cannot read umask\n");
		exit(1);
	}
	fclose(fp);
}