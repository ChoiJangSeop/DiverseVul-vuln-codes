static int open_shell(void) {
	EUID_ASSERT();
	assert(cfg.shell);

	if (arg_debug)
		printf("Opening shell %s\n", cfg.shell);
	// file descriptor will leak if not opened with O_CLOEXEC !!
	int fd = open(cfg.shell, O_PATH|O_CLOEXEC);
	if (fd == -1) {
		fprintf(stderr, "Error: cannot open shell %s\n", cfg.shell);
		exit(1);
	}

	// pass file descriptor through to the final fexecve
	if (asprintf(&cfg.keep_fd, "%s,%d", cfg.keep_fd ? cfg.keep_fd : "", fd) == -1)
		errExit("asprintf");

	return fd;
}