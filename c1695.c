	void verifyDirectoryPermissions(const string &path) {
		TRACE_POINT();
		struct stat buf;

		if (stat(path.c_str(), &buf) == -1) {
			int e = errno;
			throw FileSystemException("Cannot stat() " + path, e, path);
		} else if (buf.st_mode != (S_IFDIR | parseModeString("u=rwx,g=rx,o=rx"))) {
			throw RuntimeException("Tried to reuse existing server instance directory " +
				path + ", but it has wrong permissions");
		} else if (buf.st_uid != geteuid() || buf.st_gid != getegid()) {
			/* The server instance directory is always created by the Watchdog. Its UID/GID never
			 * changes because:
			 * 1. Disabling user switching only lowers the privilege of the HelperAgent.
			 * 2. For the UID/GID to change, the web server must be completely restarted
			 *    (not just graceful reload) so that the control process can change its UID/GID.
			 *    This causes the PID to change, so that an entirely new server instance
			 *    directory is created.
			 */
			throw RuntimeException("Tried to reuse existing server instance directory " +
				path + ", but it has wrong owner and group");
		}
	}