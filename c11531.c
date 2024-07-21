int join_namespace(pid_t pid, char *type) {
	char *path;
	if (asprintf(&path, "/proc/%u/ns/%s", pid, type) == -1)
		errExit("asprintf");

	int fd = open(path, O_RDONLY);
	if (fd < 0)
		goto errout;

	if (syscall(__NR_setns, fd, 0) < 0) {
		close(fd);
		goto errout;
	}

	close(fd);
	free(path);
	return 0;

errout:
	free(path);
	fprintf(stderr, "Error: cannot join namespace %s\n", type);
	return -1;

}