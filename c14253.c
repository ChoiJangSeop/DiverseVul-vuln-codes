static int sc_open_snapd_tool(const char *tool_name)
{
	// +1 is for the case where the link is exactly PATH_MAX long but we also
	// want to store the terminating '\0'. The readlink system call doesn't add
	// terminating null, but our initialization of buf handles this for us.
	char buf[PATH_MAX + 1] = { 0 };
	if (readlink("/proc/self/exe", buf, sizeof buf) < 0) {
		die("cannot readlink /proc/self/exe");
	}
	if (buf[0] != '/') {	// this shouldn't happen, but make sure have absolute path
		die("readlink /proc/self/exe returned relative path");
	}
	char *dir_name = dirname(buf);
	int dir_fd SC_CLEANUP(sc_cleanup_close) = 1;
	dir_fd = open(dir_name, O_PATH | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
	if (dir_fd < 0) {
		die("cannot open path %s", dir_name);
	}
	int tool_fd = -1;
	tool_fd = openat(dir_fd, tool_name, O_PATH | O_NOFOLLOW | O_CLOEXEC);
	if (tool_fd < 0) {
		die("cannot open path %s/%s", dir_name, tool_name);
	}
	debug("opened %s executable as file descriptor %d", tool_name, tool_fd);
	return tool_fd;
}