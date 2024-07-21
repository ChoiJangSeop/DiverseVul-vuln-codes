getFileTypeNoFollowSymlinks(const StaticString &filename) {
	struct stat buf;
	int ret;
	
	ret = lstat(filename.c_str(), &buf);
	if (ret == 0) {
		if (S_ISREG(buf.st_mode)) {
			return FT_REGULAR;
		} else if (S_ISDIR(buf.st_mode)) {
			return FT_DIRECTORY;
		} else if (S_ISLNK(buf.st_mode)) {
			return FT_SYMLINK;
		} else {
			return FT_OTHER;
		}
	} else {
		if (errno == ENOENT) {
			return FT_NONEXISTANT;
		} else {
			int e = errno;
			string message("Cannot lstat '");
			message.append(filename);
			message.append("'");
			throw FileSystemException(message, e, filename);
		}
	}
}