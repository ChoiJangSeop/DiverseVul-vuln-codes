int process_stat(ProcessHandle process, const char *fname, struct stat *s) {
	int rv = process_stat_nofail(process, fname, s);
	if (rv) {
		fprintf(stderr, "Error: cannot stat /proc/%d/%s: %s\n", process->pid, fname, strerror(errno));
		exit(1);
	}

	return rv;
}