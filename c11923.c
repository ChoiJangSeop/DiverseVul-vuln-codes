static int utab_lock(const char *filename)
{
	char *lfile;
	int fd;

	assert(filename);

	if (asprintf(&lfile, "%s.lock", filename) == -1)
		return -1;

	DBG(UPDATE, mnt_debug("%s: locking", lfile));

	fd = open(lfile, O_RDONLY|O_CREAT|O_CLOEXEC, S_IWUSR|
			                             S_IRUSR|S_IRGRP|S_IROTH);
	free(lfile);

	if (fd < 0)
		return -errno;

	while (flock(fd, LOCK_EX) < 0) {
		int errsv;
		if (errno == EINTR)
			continue;
		errsv = errno;
		close(fd);
		return -errsv;
	}
	return fd;
}