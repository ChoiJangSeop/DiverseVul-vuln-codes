void sandboxfs(int op, pid_t pid, const char *path1, const char *path2) {
	EUID_ASSERT();
	assert(path1);

	// in case the pid is that of a firejail process, use the pid of the first child process
	pid = switch_to_child(pid);

	// exit if no permission to join the sandbox
	check_join_permission(pid);

	// expand paths
	char *fname1 = expand_path(path1);
	char *fname2 = NULL;
	if (path2 != NULL) {
		fname2 = expand_path(path2);
	}
	if (arg_debug) {
		printf("file1 %s\n", fname1);
		printf("file2 %s\n", fname2 ? fname2 : "(null)");
	}

	// get file from sandbox and store it in the current directory
	// implemented using --cat
	if (op == SANDBOX_FS_GET) {
		char *dest_fname = strrchr(fname1, '/');
		if (!dest_fname || *(++dest_fname) == '\0') {
			fprintf(stderr, "Error: invalid file name %s\n", fname1);
			exit(1);
		}
		// create destination file if necessary
		EUID_ASSERT();
		int fd = open(dest_fname, O_WRONLY|O_CREAT|O_CLOEXEC, S_IRUSR | S_IWUSR);
		if (fd == -1) {
			fprintf(stderr, "Error: cannot open %s for writing\n", dest_fname);
			exit(1);
		}
		struct stat s;
		if (fstat(fd, &s) == -1)
			errExit("fstat");
		if (!S_ISREG(s.st_mode)) {
			fprintf(stderr, "Error: %s is no regular file\n", dest_fname);
			exit(1);
		}
		if (ftruncate(fd, 0) == -1)
			errExit("ftruncate");
		// go quiet - messages on stdout will corrupt the file
		arg_debug = 0;
		arg_quiet = 1;
		// redirection
		if (dup2(fd, STDOUT_FILENO) == -1)
			errExit("dup2");
		close(fd);
		op = SANDBOX_FS_CAT;
	}

	// sandbox root directory
	char *rootdir;
	if (asprintf(&rootdir, "/proc/%d/root", pid) == -1)
		errExit("asprintf");

	if (op == SANDBOX_FS_LS || op == SANDBOX_FS_CAT) {
		EUID_ROOT();
		// chroot
		if (chroot(rootdir) < 0)
			errExit("chroot");
		if (chdir("/") < 0)
			errExit("chdir");

		// drop privileges
		drop_privs(0);

		if (op == SANDBOX_FS_LS)
			ls(fname1);
		else
			cat(fname1);

		__gcov_flush();
	}
	// get file from host and store it in the sandbox
	else if (op == SANDBOX_FS_PUT && path2) {
		char *src_fname =fname1;
		char *dest_fname = fname2;

		EUID_ROOT();
		if (arg_debug)
			printf("copy %s to %s\n", src_fname, dest_fname);

		// create a user-owned temporary file in /run/firejail directory
		char tmp_fname[] = "/run/firejail/tmpget-XXXXXX";
		int fd = mkstemp(tmp_fname);
		if (fd == -1) {
			fprintf(stderr, "Error: cannot create temporary file %s\n", tmp_fname);
			exit(1);
		}
		SET_PERMS_FD(fd, getuid(), getgid(), 0600);
		close(fd);

		// copy the source file into the temporary file - we need to chroot
		pid_t child = fork();
		if (child < 0)
			errExit("fork");
		if (child == 0) {
			// drop privileges
			drop_privs(0);

			// copy the file
			if (copy_file(src_fname, tmp_fname, getuid(), getgid(), 0600)) // already a regular user
				_exit(1);

			__gcov_flush();

			_exit(0);
		}

		// wait for the child to finish
		int status = 0;
		waitpid(child, &status, 0);
		if (WIFEXITED(status) && WEXITSTATUS(status) == 0);
		else {
			unlink(tmp_fname);
			exit(1);
		}

		// copy the temporary file into the destination file
		child = fork();
		if (child < 0)
			errExit("fork");
		if (child == 0) {
			// chroot
			if (chroot(rootdir) < 0)
				errExit("chroot");
			if (chdir("/") < 0)
				errExit("chdir");

			// drop privileges
			drop_privs(0);

			// copy the file
			if (copy_file(tmp_fname, dest_fname, getuid(), getgid(), 0600)) // already a regular user
				_exit(1);

			__gcov_flush();

			_exit(0);
		}

		// wait for the child to finish
		status = 0;
		waitpid(child, &status, 0);
		if (WIFEXITED(status) && WEXITSTATUS(status) == 0);
		else {
			unlink(tmp_fname);
			exit(1);
		}

		// remove the temporary file
		unlink(tmp_fname);
		EUID_USER();
	}

	if (fname2)
		free(fname2);
	free(fname1);
	free(rootdir);

	exit(0);
}