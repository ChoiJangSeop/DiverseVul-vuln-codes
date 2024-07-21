	void initialize(const string &path, bool owner) {
		TRACE_POINT();
		this->path  = path;
		this->owner = owner;
		
		/* Create the server instance directory. We only need to write to this
		 * directory for these reasons:
		 * 1. Initial population of structure files (structure_version.txt, instance.pid).
		 * 2. Creating/removing a generation directory.
		 * 3. Removing the entire server instance directory (after all
		 *    generations are removed).
		 *
		 * 1 and 2 are done by the helper server during initialization and before lowering
		 * privilege. 3 is done during helper server shutdown by a cleanup process that's
		 * running as the same user the helper server was running as before privilege
		 * lowering.
		 * Therefore, we make the directory only writable by the user the helper server
		 * was running as before privilege is lowered. Everybody else has read and execute
		 * rights though, because we want admin tools to be able to list the available
		 * generations no matter what user they're running as.
		 */
		makeDirTree(path, "u=rwx,g=rx,o=rx");
	}