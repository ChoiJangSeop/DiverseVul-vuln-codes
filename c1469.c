	void createDirectory(const string &path) const {
		// We do not use makeDirTree() here. If an attacker creates a directory
		// just before we do, then we want to abort because we want the directory
		// to have specific permissions.
		if (mkdir(path.c_str(), parseModeString("u=rwx,g=rx,o=rx")) == -1) {
			int e = errno;
			throw FileSystemException("Cannot create server instance directory '" +
				path + "'", e, path);
		}
	}