FILE *fopen_safe(const char *path, const char *mode)
{
	int fd;
	FILE *file;
	int flags = O_NOFOLLOW | O_CREAT;

	if (mode[0] == 'r')
		return fopen(path, mode);

	if (mode[0] != 'a' && mode[0] != 'w')
		return NULL;

	if (mode[1] &&
	    (mode[1] != '+' || mode[2]))
		return NULL;

	if (mode[0] == 'w')
		flags |= O_TRUNC;
	else
		flags |= O_APPEND;

	if (mode[1])
		flags |= O_RDWR;
	else
		flags |= O_WRONLY;

	fd = open(path, flags, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
	if (fd == -1)
		return NULL;

	file = fdopen (fd, "w");
	if (!file) {
		close(fd);
		return NULL;
	}

	return file;
}