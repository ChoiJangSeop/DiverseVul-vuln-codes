bool is_ready_for_join(const pid_t pid) {
	EUID_ASSERT();
	// check if a file /run/firejail/mnt/join exists
	char *fname;
	if (asprintf(&fname, "/proc/%d/root%s", pid, RUN_JOIN_FILE) == -1)
		errExit("asprintf");
	EUID_ROOT();
	int fd = open(fname, O_RDONLY|O_CLOEXEC);
	EUID_USER();
	free(fname);
	if (fd == -1)
		return false;
	struct stat s;
	if (fstat(fd, &s) == -1)
		errExit("fstat");
	if (!S_ISREG(s.st_mode) || s.st_uid != 0 || s.st_size != 1) {
		close(fd);
		return false;
	}
	char status;
	if (read(fd, &status, 1) == 1 && status == SANDBOX_DONE) {
		close(fd);
		return true;
	}
	close(fd);
	return false;
}