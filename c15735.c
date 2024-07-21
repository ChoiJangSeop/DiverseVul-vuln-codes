void bandwidth_pid(pid_t pid, const char *command, const char *dev, int down, int up) {
	EUID_ASSERT();
	//************************
	// verify sandbox
	//************************
	EUID_ROOT();
	char *comm = pid_proc_comm(pid);
	EUID_USER();
	if (!comm) {
		fprintf(stderr, "Error: cannot find sandbox\n");
		exit(1);
	}

	// check for firejail sandbox
	if (strcmp(comm, "firejail") != 0) {
		fprintf(stderr, "Error: cannot find sandbox\n");
		exit(1);
	}
	free(comm);
	
	// check network namespace
	char *name;
	if (asprintf(&name, "/run/firejail/network/%d-netmap", pid) == -1)
		errExit("asprintf");
	struct stat s;
	if (stat(name, &s) == -1) {
		fprintf(stderr, "Error: the sandbox doesn't use a new network namespace\n");
		exit(1);
	}

	//************************
	// join the network namespace
	//************************
	pid_t child;
	if (find_child(pid, &child) == -1) {
		fprintf(stderr, "Error: cannot join the network namespace\n");
		exit(1);
	}

	EUID_ROOT();
	if (join_namespace(child, "net")) {
		fprintf(stderr, "Error: cannot join the network namespace\n");
		exit(1);
	}

	// set run file
	if (strcmp(command, "set") == 0)
		bandwidth_set(pid, dev, down, up);
	else if (strcmp(command, "clear") == 0)
		bandwidth_remove(pid, dev);

	//************************
	// build command
	//************************
	char *devname = NULL;
	if (dev) {
		// read network map file
		char *fname;
		if (asprintf(&fname, "%s/%d-netmap", RUN_FIREJAIL_NETWORK_DIR, (int) pid) == -1)
			errExit("asprintf");
		FILE *fp = fopen(fname, "r");
		if (!fp) {
			fprintf(stderr, "Error: cannot read network map file %s\n", fname);
			exit(1);
		}
		
		char buf[1024];
		int len = strlen(dev);
		while (fgets(buf, 1024, fp)) {
			// remove '\n'
			char *ptr = strchr(buf, '\n');
			if (ptr)
				*ptr = '\0';
			if (*buf == '\0')
				break;

			if (strncmp(buf, dev, len) == 0  && buf[len] == ':') {
				devname = strdup(buf + len + 1);
				if (!devname)
					errExit("strdup");
				// check device in namespace
				if (if_nametoindex(devname) == 0) {
					fprintf(stderr, "Error: cannot find network device %s\n", devname);
					exit(1);
				}
				break;
			}
		}
		free(fname);
		fclose(fp);
	}
	
	// build fshaper.sh command
	char *cmd = NULL;
	if (devname) {
		if (strcmp(command, "set") == 0) {
			if (asprintf(&cmd, "%s/firejail/fshaper.sh --%s %s %d %d",
				LIBDIR, command, devname, down, up) == -1)
				errExit("asprintf");
		}
		else {
			if (asprintf(&cmd, "%s/firejail/fshaper.sh --%s %s",
				LIBDIR, command, devname) == -1)
				errExit("asprintf");
		}
	}
	else {
		if (asprintf(&cmd, "%s/firejail/fshaper.sh --%s", LIBDIR, command) == -1)
			errExit("asprintf");
	}
	assert(cmd);

	// wipe out environment variables
	environ = NULL;

	//************************
	// build command
	//************************
	// elevate privileges
	if (setreuid(0, 0))
		errExit("setreuid");
	if (setregid(0, 0))
		errExit("setregid");

	if (!cfg.shell)
		cfg.shell = guess_shell();
	if (!cfg.shell) {
		fprintf(stderr, "Error: no POSIX shell found, please use --shell command line option\n");
		exit(1);
	}

	char *arg[4];
	arg[0] = cfg.shell;
	arg[1] = "-c";
	arg[2] = cmd;
	arg[3] = NULL;
	clearenv();
	execvp(arg[0], arg);
	
	// it will never get here
	errExit("execvp");
}