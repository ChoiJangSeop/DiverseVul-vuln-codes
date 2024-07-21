void fs_logger_print_log(pid_t pid) {
	EUID_ASSERT();

	// in case the pid is that of a firejail process, use the pid of the first child process
	pid = switch_to_child(pid);

	// exit if no permission to join the sandbox
	check_join_permission(pid);

	// print RUN_FSLOGGER_FILE
	char *fname;
	if (asprintf(&fname, "/proc/%d/root%s", pid, RUN_FSLOGGER_FILE) == -1)
		errExit("asprintf");

	EUID_ROOT();
	FILE *fp = fopen(fname, "re");
	free(fname);
	if (!fp) {
		fprintf(stderr, "Error: Cannot open filesystem log\n");
		exit(1);
	}
	char buf[MAXBUF];
	while (fgets(buf, MAXBUF, fp))
		printf("%s", buf);
	fclose(fp);

	exit(0);
}