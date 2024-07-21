pid_t switch_to_child(pid_t pid) {
	EUID_ASSERT();
	EUID_ROOT();
	pid_t rv = pid;
	errno = 0;
	char *comm = pid_proc_comm(pid);
	if (!comm) {
		if (errno == ENOENT)
			fprintf(stderr, "Error: cannot find process with pid %d\n", pid);
		else
			fprintf(stderr, "Error: cannot read /proc file\n");
		exit(1);
	}
	EUID_USER();

	if (strcmp(comm, "firejail") == 0) {
		if (find_child(pid, &rv) == 1) {
			fprintf(stderr, "Error: no valid sandbox\n");
			exit(1);
		}
		fmessage("Switching to pid %u, the first child process inside the sandbox\n", (unsigned) rv);
	}
	free(comm);
	return rv;
}