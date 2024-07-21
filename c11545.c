int process_open(ProcessHandle process, const char *fname) {
	int rv = process_open_nofail(process, fname);
	if (rv < 0) {
		fprintf(stderr, "Error: cannot open /proc/%d/%s: %s\n", process->pid, fname, strerror(errno));
		exit(1);
	}

	return rv;
}