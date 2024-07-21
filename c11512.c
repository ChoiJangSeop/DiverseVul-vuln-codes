static void extract_apparmor(pid_t pid) {
	if (checkcfg(CFG_APPARMOR)) {
		EUID_USER();
		if (aa_is_enabled() == 1) {
			// get pid of next child process
			pid_t child;
			if (find_child(pid, &child) == 1)
				child = pid; // no child, proceed with current pid

			// get name of AppArmor profile
			char *fname;
			if (asprintf(&fname, "/proc/%d/attr/current", child) == -1)
				errExit("asprintf");
			EUID_ROOT();
			int fd = open(fname, O_RDONLY|O_CLOEXEC);
			EUID_USER();
			free(fname);
			if (fd == -1)
				goto errexit;
			char buf[BUFLEN];
			ssize_t rv = read(fd, buf, sizeof(buf) - 1);
			close(fd);
			if (rv < 0)
				goto errexit;
			buf[rv] = '\0';
			// process confined by Firejail's AppArmor policy?
			if (strncmp(buf, "firejail-default", 16) == 0)
				arg_apparmor = 1;
		}
		EUID_ROOT();
	}
	return;

errexit:
	fprintf(stderr, "Error: cannot read /proc file\n");
	exit(1);
}