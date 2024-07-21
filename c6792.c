write_stacktrace(const char *file_name, const char *str)
{
	int fd;
	void *buffer[100];
	int nptrs;
	int i;
	char **strs;

	nptrs = backtrace(buffer, 100);
	if (file_name) {
		fd = open(file_name, O_WRONLY | O_APPEND | O_CREAT, 0644);
		if (str)
			dprintf(fd, "%s\n", str);
		backtrace_symbols_fd(buffer, nptrs, fd);
		if (write(fd, "\n", 1) != 1) {
			/* We don't care, but this stops a warning on Ubuntu */
		}
		close(fd);
	} else {
		if (str)
			log_message(LOG_INFO, "%s", str);
		strs = backtrace_symbols(buffer, nptrs);
		if (strs == NULL) {
			log_message(LOG_INFO, "Unable to get stack backtrace");
			return;
		}

		/* We don't need the call to this function, or the first two entries on the stack */
		for (i = 1; i < nptrs - 2; i++)
			log_message(LOG_INFO, "  %s", strs[i]);
		free(strs);
	}
}