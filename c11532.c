void join(pid_t pid, int argc, char **argv, int index) {
	EUID_ASSERT();

	pid_t parent = pid;
	// in case the pid is that of a firejail process, use the pid of the first child process
	pid = switch_to_child(pid);

	// exit if no permission to join the sandbox
	check_join_permission(pid);

	extract_x11_display(parent);

	int shfd = -1;
	if (!arg_shell_none)
		shfd = open_shell();

	EUID_ROOT();
	// in user mode set caps seccomp, cpu, cgroup, etc
	if (getuid() != 0) {
		extract_nonewprivs(pid);  // redundant on Linux >= 4.10; duplicated in function extract_caps
		extract_caps(pid);
		extract_cpu(pid);
		extract_cgroup(pid);
		extract_nogroups(pid);
		extract_user_namespace(pid);
		extract_umask(pid);
#ifdef HAVE_APPARMOR
		extract_apparmor(pid);
#endif
	}

	// set cgroup
	if (cfg.cgroup)	// not available for uid 0
		set_cgroup(cfg.cgroup, getpid());

	// join namespaces
	if (arg_join_network) {
		if (join_namespace(pid, "net"))
			exit(1);
	}
	else if (arg_join_filesystem) {
		if (join_namespace(pid, "mnt"))
			exit(1);
	}
	else {
		if (join_namespace(pid, "ipc") ||
		    join_namespace(pid, "net") ||
		    join_namespace(pid, "pid") ||
		    join_namespace(pid, "uts") ||
		    join_namespace(pid, "mnt"))
			exit(1);
	}

	pid_t child = fork();
	if (child < 0)
		errExit("fork");
	if (child == 0) {
		// drop discretionary access control capabilities for root sandboxes
		caps_drop_dac_override();

		// chroot into /proc/PID/root directory
		char *rootdir;
		if (asprintf(&rootdir, "/proc/%d/root", pid) == -1)
			errExit("asprintf");

		int rv;
		if (!arg_join_network) {
			rv = chroot(rootdir); // this will fail for processes in sandboxes not started with --chroot option
			if (rv == 0)
				printf("changing root to %s\n", rootdir);
		}

		EUID_USER();
		if (chdir("/") < 0)
			errExit("chdir");
		if (cfg.homedir) {
			struct stat s;
			if (stat(cfg.homedir, &s) == 0) {
				/* coverity[toctou] */
				if (chdir(cfg.homedir) < 0)
					errExit("chdir");
			}
		}

		// set caps filter
		EUID_ROOT();
		if (apply_caps == 1)	// not available for uid 0
			caps_set(caps);
		if (getuid() != 0)
			seccomp_load_file_list();

		// mount user namespace or drop privileges
		if (arg_noroot) {	// not available for uid 0
			if (arg_debug)
				printf("Joining user namespace\n");
			if (join_namespace(1, "user"))
				exit(1);

			// user namespace resets capabilities
			// set caps filter
			if (apply_caps == 1)	// not available for uid 0
				caps_set(caps);
		}

		// set nonewprivs
		if (arg_nonewprivs == 1) {	// not available for uid 0
			int rv = prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0);
			if (arg_debug && rv == 0)
				printf("NO_NEW_PRIVS set\n");
		}

		EUID_USER();
		int cwd = 0;
		if (cfg.cwd) {
			if (chdir(cfg.cwd) == 0)
				cwd = 1;
		}

		if (!cwd) {
			if (chdir("/") < 0)
				errExit("chdir");
			if (cfg.homedir) {
				struct stat s;
				if (stat(cfg.homedir, &s) == 0) {
					/* coverity[toctou] */
					if (chdir(cfg.homedir) < 0)
						errExit("chdir");
				}
			}
		}

		// drop privileges
		drop_privs(arg_nogroups);

		// kill the child in case the parent died
		prctl(PR_SET_PDEATHSIG, SIGKILL, 0, 0, 0);

#ifdef HAVE_APPARMOR
		set_apparmor();
#endif

		extract_command(argc, argv, index);
		if (cfg.command_line == NULL) {
			assert(cfg.shell);
			cfg.window_title = cfg.shell;
		}
		else if (arg_debug)
			printf("Extracted command #%s#\n", cfg.command_line);

		// set cpu affinity
		if (cfg.cpus)	// not available for uid 0
			set_cpu_affinity();

		// add x11 display
		if (display) {
			char *display_str;
			if (asprintf(&display_str, ":%d", display) == -1)
				errExit("asprintf");
			env_store_name_val("DISPLAY", display_str, SETENV);
			free(display_str);
		}

#ifdef HAVE_DBUSPROXY
		// set D-Bus environment variables
		struct stat s;
		if (stat(RUN_DBUS_USER_SOCKET, &s) == 0)
			dbus_set_session_bus_env();
		if (stat(RUN_DBUS_SYSTEM_SOCKET, &s) == 0)
			dbus_set_system_bus_env();
#endif

		start_application(0, shfd, NULL);

		__builtin_unreachable();
	}
	EUID_USER();
	if (shfd != -1)
		close(shfd);

	int status = 0;
	//*****************************
	// following code is signal-safe

	install_handler();

	// wait for the child to finish
	waitpid(child, &status, 0);

	// restore default signal action
	signal(SIGTERM, SIG_DFL);

	// end of signal-safe code
	//*****************************

	if (WIFEXITED(status)) {
		// if we had a proper exit, return that exit status
		status = WEXITSTATUS(status);
	} else if (WIFSIGNALED(status)) {
		// distinguish fatal signals by adding 128
		status = 128 + WTERMSIG(status);
	} else {
		status = -1;
	}

	flush_stdin();
	exit(status);
}