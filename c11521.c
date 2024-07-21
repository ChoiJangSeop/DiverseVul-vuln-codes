void protocol_print_filter(pid_t pid) {
	EUID_ASSERT();

	(void) pid;
#ifdef SYS_socket
	// in case the pid is that of a firejail process, use the pid of the first child process
	pid = switch_to_child(pid);

	// exit if no permission to join the sandbox
	check_join_permission(pid);

	// find the seccomp filter
	EUID_ROOT();
	char *fname;
	if (asprintf(&fname, "/proc/%d/root%s", pid, RUN_PROTOCOL_CFG) == -1)
		errExit("asprintf");

	struct stat s;
	if (stat(fname, &s) == -1) {
		printf("Cannot access seccomp filter.\n");
		exit(1);
	}

	// read and print the filter
	protocol_filter_load(fname);
	free(fname);
	if (cfg.protocol)
		printf("%s\n", cfg.protocol);
	exit(0);
#else
	fwarning("--protocol not supported on this platform\n");
	exit(1);
#endif
}