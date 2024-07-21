int main(int argc, char **argv, char **envp) {
	int i;
	int prog_index = -1;			  // index in argv where the program command starts
	int lockfd_network = -1;
	int lockfd_directory = -1;
	int option_cgroup = 0;
	int custom_profile = 0;	// custom profile loaded
	int arg_caps_cmdline = 0; 	// caps requested on command line (used to break out of --chroot)
	int arg_netlock = 0;
	char **ptr;


	// sanitize the umask
	orig_umask = umask(022);

	// drop permissions by default and rise them when required
	EUID_INIT();
	EUID_USER();

	// check standard streams before opening any file
	fix_std_streams();

	// argument count should be larger than 0
	if (argc == 0 || !argv || strlen(argv[0]) == 0) {
		fprintf(stderr, "Error: argv is invalid\n");
		exit(1);
	} else if (argc >= MAX_ARGS) {
		fprintf(stderr, "Error: too many arguments: argc (%d) >= MAX_ARGS (%d)\n", argc, MAX_ARGS);
		exit(1);
	}

	// sanity check for arguments
	for (i = 0; i < argc; i++) {
		if (strlen(argv[i]) >= MAX_ARG_LEN) {
			fprintf(stderr, "Error: too long arguments: argv[%d] len (%zu) >= MAX_ARG_LEN (%d)\n", i, strlen(argv[i]), MAX_ARG_LEN);
			exit(1);
		}
	}

	// Stash environment variables
	for (i = 0, ptr = envp; ptr && *ptr && i < MAX_ENVS; i++, ptr++)
		env_store(*ptr, SETENV);

	// sanity check for environment variables
	if (i >= MAX_ENVS) {
		fprintf(stderr, "Error: too many environment variables: >= MAX_ENVS (%d)\n", MAX_ENVS);
		exit(1);
	}

	// Reapply a minimal set of environment variables
	env_apply_whitelist();

	// process --quiet
	const char *env_quiet = env_get("FIREJAIL_QUIET");
	if (check_arg(argc, argv, "--quiet", 1) || (env_quiet && strcmp(env_quiet, "yes") == 0))
		arg_quiet = 1;

	// check if the user is allowed to use firejail
	init_cfg(argc, argv);

	// get starting timestamp
	timetrace_start();

	// check argv[0] symlink wrapper if this is not a login shell
	if (*argv[0] != '-')
		run_symlink(argc, argv, 0); // if symlink detected, this function will not return

	// check if we already have a sandbox running
	// If LXC is detected, start firejail sandbox
	// otherwise try to detect a PID namespace by looking under /proc for specific kernel processes and:
	//	- start the application in a /bin/bash shell
	if (check_namespace_virt() == 0) {
		EUID_ROOT();
		int rv = check_kernel_procs();
		EUID_USER();
		if (rv == 0) {
			if (check_arg(argc, argv, "--version", 1)) {
				print_version();
				exit(0);
			}

			// start the program directly without sandboxing
			run_no_sandbox(argc, argv);
			__builtin_unreachable();
		}
	}

	// profile builder
	if (check_arg(argc, argv, "--build", 0)) // supports both --build and --build=filename
		run_builder(argc, argv); // this function will not return

	// intrusion detection system
#ifdef HAVE_IDS
	if (check_arg(argc, argv, "--ids-", 0)) // supports both --ids-init and --ids-check
		run_ids(argc, argv); // this function will not return
#else
	if (check_arg(argc, argv, "--ids-", 0)) { // supports both --ids-init and --ids-check
		fprintf(stderr, "Error: IDS features disabled in your Firejail build.\n"
			"\tTo enable it, configure your build system using --enable-ids.\n"
			"\tExample: ./configure --prefix=/usr --enable-ids\n\n");
		exit(1);
	}
#endif

	EUID_ROOT();
#ifndef HAVE_SUID
	if (geteuid() != 0) {
		fprintf(stderr, "Error: Firejail needs to be SUID.\n");
		fprintf(stderr, "Assuming firejail is installed in /usr/bin, execute the following command as root:\n");
		fprintf(stderr, "  chmod u+s /usr/bin/firejail\n");
	}
#endif

	// build /run/firejail directory structure
	preproc_build_firejail_dir();
	const char *container_name = env_get("container");
	if (!container_name || strcmp(container_name, "firejail")) {
		lockfd_directory = open(RUN_DIRECTORY_LOCK_FILE, O_WRONLY | O_CREAT | O_CLOEXEC, S_IRUSR | S_IWUSR);
		if (lockfd_directory != -1) {
			int rv = fchown(lockfd_directory, 0, 0);
			(void) rv;
			flock(lockfd_directory, LOCK_EX);
		}
		preproc_clean_run();
		flock(lockfd_directory, LOCK_UN);
		close(lockfd_directory);
	}

	delete_run_files(getpid());
	atexit(clear_atexit);
	EUID_USER();

	// check if the parent is sshd daemon
	int parent_sshd = 0;
	{
		pid_t ppid = getppid();
		EUID_ROOT();
		char *comm = pid_proc_comm(ppid);
		EUID_USER();
		if (comm) {
			if (strcmp(comm, "sshd") == 0) {
				arg_quiet = 1;
				parent_sshd = 1;

#ifdef DEBUG_RESTRICTED_SHELL
				{EUID_ROOT();
				FILE *fp = fopen("/firelog", "we");
				if (fp) {
					int i;
					fprintf(fp, "argc %d: ", argc);
					for (i = 0; i < argc; i++)
						fprintf(fp, "#%s# ", argv[i]);
					fprintf(fp, "\n");
					fclose(fp);
				}
				EUID_USER();}
#endif
				// run sftp and scp directly without any sandboxing
				// regular login has argv[0] == "-firejail"
				if (*argv[0] != '-') {
					if (strcmp(argv[1], "-c") == 0 && argc > 2) {
						if (strcmp(argv[2], "/usr/lib/openssh/sftp-server") == 0 ||
						    strncmp(argv[2], "scp ", 4) == 0) {
#ifdef DEBUG_RESTRICTED_SHELL
							{EUID_ROOT();
							FILE *fp = fopen("/firelog", "ae");
							if (fp) {
								fprintf(fp, "run without a sandbox\n");
								fclose(fp);
							}
							EUID_USER();}
#endif

							drop_privs(1);
							umask(orig_umask);

							// restore original environment variables
							env_apply_all();
							int rv = system(argv[2]);
							exit(rv);
						}
					}
				}
			}
			free(comm);
		}
	}
	EUID_ASSERT();

	// is this a login shell, or a command passed by sshd,
	// insert command line options from /etc/firejail/login.users
	if (*argv[0] == '-' || parent_sshd) {
		if (argc == 1)
			login_shell = 1;
		fullargc = restricted_shell(cfg.username);
		if (fullargc) {

#ifdef DEBUG_RESTRICTED_SHELL
			{EUID_ROOT();
			FILE *fp = fopen("/firelog", "ae");
			if (fp) {
				fprintf(fp, "fullargc %d: ",  fullargc);
				int i;
				for (i = 0; i < fullargc; i++)
					fprintf(fp, "#%s# ", fullargv[i]);
				fprintf(fp, "\n");
				fclose(fp);
			}
			EUID_USER();}
#endif

			int j;
			for (i = 1, j = fullargc; i < argc && j < MAX_ARGS; i++, j++, fullargc++)
				fullargv[j] = argv[i];

			// replace argc/argv with fullargc/fullargv
			argv = fullargv;
			argc = j;

#ifdef DEBUG_RESTRICTED_SHELL
			{EUID_ROOT();
			FILE *fp = fopen("/firelog", "ae");
			if (fp) {
				fprintf(fp, "argc %d: ", argc);
				int i;
				for (i = 0; i < argc; i++)
					fprintf(fp, "#%s# ", argv[i]);
				fprintf(fp, "\n");
				fclose(fp);
			}
			EUID_USER();}
#endif
		}
	}
#ifdef HAVE_OUTPUT
	else {
		// check --output option and execute it;
		check_output(argc, argv); // the function will not return if --output or --output-stderr option was found
	}
#endif
	EUID_ASSERT();

	// --ip=dhcp - we need access to /sbin and /usr/sbin directories in order to run ISC DHCP client (dhclient)
	// these paths are disabled in disable-common.inc
	if ((i = check_arg(argc, argv, "--ip", 0)) != 0) {
		if (strncmp(argv[i] + 4, "=dhcp", 5) == 0) {
			profile_add("noblacklist /sbin");
			profile_add("noblacklist /usr/sbin");
		}
	}

	// process allow-debuggers
	if (check_arg(argc, argv, "--allow-debuggers", 1)) {
		// check kernel version
		struct utsname u;
		int rv = uname(&u);
		if (rv != 0)
			errExit("uname");
		int major;
		int minor;
		if (2 != sscanf(u.release, "%d.%d", &major, &minor)) {
			fprintf(stderr, "Error: cannot extract Linux kernel version: %s\n", u.version);
			exit(1);
		}
		if (major < 4 || (major == 4 && minor < 8)) {
			fprintf(stderr, "Error: --allow-debuggers is disabled on Linux kernels prior to 4.8. "
				"A bug in ptrace call allows a full bypass of the seccomp filter. "
				"Your current kernel version is %d.%d.\n", major, minor);
			exit(1);
		}

		arg_allow_debuggers = 1;
		char *cmd = strdup("noblacklist ${PATH}/strace");
		if (!cmd)
			errExit("strdup");
		profile_add(cmd);
	}

	// for appimages we need to remove "include disable-shell.inc from the profile
	// a --profile command can show up before --appimage
	if (check_arg(argc, argv, "--appimage", 1))
		arg_appimage = 1;

	// check for force-nonewprivs in /etc/firejail/firejail.config file
	if (checkcfg(CFG_FORCE_NONEWPRIVS))
		arg_nonewprivs = 1;

	// check oom
	if ((i = check_arg(argc, argv, "--oom=", 0)) != 0)
		oom_set(argv[i] + 6);

	// parse arguments
	for (i = 1; i < argc; i++) {
		run_cmd_and_exit(i, argc, argv); // will exit if the command is recognized

		if (strcmp(argv[i], "--debug") == 0) {
			arg_debug = 1;
			arg_quiet = 0;
		}
		else if (strcmp(argv[i], "--debug-blacklists") == 0)
			arg_debug_blacklists = 1;
		else if (strcmp(argv[i], "--debug-whitelists") == 0)
			arg_debug_whitelists = 1;
		else if (strcmp(argv[i], "--debug-private-lib") == 0)
			arg_debug_private_lib = 1;
		else if (strcmp(argv[i], "--quiet") == 0) {
			if (!arg_debug)
				arg_quiet = 1;
		}
		else if (strcmp(argv[i], "--allow-debuggers") == 0) {
			// already handled
		}


		//*************************************
		// x11
		//*************************************

#ifdef HAVE_X11
		else if (strncmp(argv[i], "--xephyr-screen=", 16) == 0) {
			if (checkcfg(CFG_X11))
				; // the processing is done directly in x11.c
			else
				exit_err_feature("x11");
		}
#endif
		//*************************************
		// filtering
		//*************************************
#ifdef HAVE_APPARMOR
		else if (strcmp(argv[i], "--apparmor") == 0)
			arg_apparmor = 1;
#endif
		else if (strncmp(argv[i], "--protocol=", 11) == 0) {
			if (checkcfg(CFG_SECCOMP)) {
				const char *add = argv[i] + 11;
				profile_list_augment(&cfg.protocol, add);
				if (arg_debug)
					fprintf(stderr, "[option] combined protocol list: \"%s\"\n", cfg.protocol);
			}
			else
				exit_err_feature("seccomp");
		}
		else if (strcmp(argv[i], "--seccomp") == 0) {
			if (checkcfg(CFG_SECCOMP)) {
				if (arg_seccomp) {
					fprintf(stderr, "Error: seccomp already enabled\n");
					exit(1);
				}
				arg_seccomp = 1;
			}
			else
				exit_err_feature("seccomp");
		}
		else if (strncmp(argv[i], "--seccomp=", 10) == 0) {
			if (checkcfg(CFG_SECCOMP)) {
				if (arg_seccomp) {
					fprintf(stderr, "Error: seccomp already enabled\n");
					exit(1);
				}
				arg_seccomp = 1;
				cfg.seccomp_list = seccomp_check_list(argv[i] + 10);
			}
			else
				exit_err_feature("seccomp");
		}
		else if (strncmp(argv[i], "--seccomp.32=", 13) == 0) {
			if (checkcfg(CFG_SECCOMP)) {
				if (arg_seccomp32) {
					fprintf(stderr, "Error: seccomp.32 already enabled\n");
					exit(1);
				}
				arg_seccomp32 = 1;
				cfg.seccomp_list32 = seccomp_check_list(argv[i] + 13);
			}
			else
				exit_err_feature("seccomp");
		}
		else if (strncmp(argv[i], "--seccomp.drop=", 15) == 0) {
			if (checkcfg(CFG_SECCOMP)) {
				if (arg_seccomp) {
					fprintf(stderr, "Error: seccomp already enabled\n");
					exit(1);
				}
				arg_seccomp = 1;
				cfg.seccomp_list_drop = seccomp_check_list(argv[i] + 15);
			}
			else
				exit_err_feature("seccomp");
		}
		else if (strncmp(argv[i], "--seccomp.32.drop=", 18) == 0) {
			if (checkcfg(CFG_SECCOMP)) {
				if (arg_seccomp32) {
					fprintf(stderr, "Error: seccomp.32 already enabled\n");
					exit(1);
				}
				arg_seccomp32 = 1;
				cfg.seccomp_list_drop32 = seccomp_check_list(argv[i] + 18);
			}
			else
				exit_err_feature("seccomp");
		}
		else if (strncmp(argv[i], "--seccomp.keep=", 15) == 0) {
			if (checkcfg(CFG_SECCOMP)) {
				if (arg_seccomp) {
					fprintf(stderr, "Error: seccomp already enabled\n");
					exit(1);
				}
				arg_seccomp = 1;
				cfg.seccomp_list_keep = seccomp_check_list(argv[i] + 15);
			}
			else
				exit_err_feature("seccomp");
		}
		else if (strncmp(argv[i], "--seccomp.32.keep=", 18) == 0) {
			if (checkcfg(CFG_SECCOMP)) {
				if (arg_seccomp32) {
					fprintf(stderr, "Error: seccomp.32 already enabled\n");
					exit(1);
				}
				arg_seccomp32 = 1;
				cfg.seccomp_list_keep32 = seccomp_check_list(argv[i] + 18);
			}
			else
				exit_err_feature("seccomp");
		}
		else if (strcmp(argv[i], "--seccomp.block-secondary") == 0) {
			if (checkcfg(CFG_SECCOMP)) {
				if (arg_seccomp32) {
					fprintf(stderr, "Error: seccomp.32 conflicts with block-secondary\n");
					exit(1);
				}
				arg_seccomp_block_secondary = 1;
			}
			else
				exit_err_feature("seccomp");
		}
		else if (strcmp(argv[i], "--memory-deny-write-execute") == 0) {
			if (checkcfg(CFG_SECCOMP))
				arg_memory_deny_write_execute = 1;
			else
				exit_err_feature("seccomp");
		}
		else if (strncmp(argv[i], "--seccomp-error-action=", 23) == 0) {
			if (checkcfg(CFG_SECCOMP)) {
				int config_seccomp_error_action = checkcfg(CFG_SECCOMP_ERROR_ACTION);
				if (config_seccomp_error_action == -1) {
					if (strcmp(argv[i] + 23, "kill") == 0)
						arg_seccomp_error_action = SECCOMP_RET_KILL;
					else if (strcmp(argv[i] + 23, "log") == 0)
						arg_seccomp_error_action = SECCOMP_RET_LOG;
					else {
						arg_seccomp_error_action = errno_find_name(argv[i] + 23);
						if (arg_seccomp_error_action == -1)
							errExit("seccomp-error-action: unknown errno");
					}
					cfg.seccomp_error_action = strdup(argv[i] + 23);
					if (!cfg.seccomp_error_action)
						errExit("strdup");
				} else
					exit_err_feature("seccomp-error-action");

			} else
				exit_err_feature("seccomp");
		}
		else if (strcmp(argv[i], "--caps") == 0) {
			arg_caps_default_filter = 1;
			arg_caps_cmdline = 1;
		}
		else if (strcmp(argv[i], "--caps.drop=all") == 0)
			arg_caps_drop_all = 1;
		else if (strncmp(argv[i], "--caps.drop=", 12) == 0) {
			arg_caps_drop = 1;
			arg_caps_list = strdup(argv[i] + 12);
			if (!arg_caps_list)
				errExit("strdup");
			// verify caps list and exit if problems
			caps_check_list(arg_caps_list, NULL);
			arg_caps_cmdline = 1;
		}
		else if (strncmp(argv[i], "--caps.keep=", 12) == 0) {
			arg_caps_keep = 1;
			arg_caps_list = strdup(argv[i] + 12);
			if (!arg_caps_list)
				errExit("strdup");
			// verify caps list and exit if problems
			caps_check_list(arg_caps_list, NULL);
			arg_caps_cmdline = 1;
		}
		else if (strcmp(argv[i], "--trace") == 0)
			arg_trace = 1;
		else if (strncmp(argv[i], "--trace=", 8) == 0) {
			arg_trace = 1;
			arg_tracefile = argv[i] + 8;
			if (*arg_tracefile == '\0') {
				fprintf(stderr, "Error: invalid trace option\n");
				exit(1);
			}
			invalid_filename(arg_tracefile, 0); // no globbing
			if (strstr(arg_tracefile, "..")) {
				fprintf(stderr, "Error: invalid file name %s\n", arg_tracefile);
				exit(1);
			}
			// if the filename starts with ~, expand the home directory
			if (*arg_tracefile == '~') {
				char *tmp;
				if (asprintf(&tmp, "%s%s", cfg.homedir, arg_tracefile + 1) == -1)
					errExit("asprintf");
				arg_tracefile = tmp;
			}
		}
		else if (strcmp(argv[i], "--tracelog") == 0)
			arg_tracelog = 1;
		else if (strncmp(argv[i], "--rlimit-cpu=", 13) == 0) {
			check_unsigned(argv[i] + 13, "Error: invalid rlimit");
			sscanf(argv[i] + 13, "%llu", &cfg.rlimit_cpu);
			arg_rlimit_cpu = 1;
		}
		else if (strncmp(argv[i], "--rlimit-nofile=", 16) == 0) {
			check_unsigned(argv[i] + 16, "Error: invalid rlimit");
			sscanf(argv[i] + 16, "%llu", &cfg.rlimit_nofile);
			arg_rlimit_nofile = 1;
		}
		else if (strncmp(argv[i], "--rlimit-nproc=", 15) == 0) {
			check_unsigned(argv[i] + 15, "Error: invalid rlimit");
			sscanf(argv[i] + 15, "%llu", &cfg.rlimit_nproc);
			arg_rlimit_nproc = 1;
		}
		else if (strncmp(argv[i], "--rlimit-fsize=", 15) == 0) {
			cfg.rlimit_fsize = parse_arg_size(argv[i] + 15);
			if (cfg.rlimit_fsize == 0) {
				perror("Error: invalid rlimit-fsize. Only use positive numbers and k, m or g suffix.");
				exit(1);
			}
			arg_rlimit_fsize = 1;
		}
		else if (strncmp(argv[i], "--rlimit-sigpending=", 20) == 0) {
			check_unsigned(argv[i] + 20, "Error: invalid rlimit");
			sscanf(argv[i] + 20, "%llu", &cfg.rlimit_sigpending);
			arg_rlimit_sigpending = 1;
		}
		else if (strncmp(argv[i], "--rlimit-as=", 12) == 0) {
			cfg.rlimit_as = parse_arg_size(argv[i] + 12);
			if (cfg.rlimit_as == 0) {
				perror("Error: invalid rlimit-as. Only use positive numbers and k, m or g suffix.");
				exit(1);
			}
			arg_rlimit_as = 1;
		}
		else if (strncmp(argv[i], "--ipc-namespace", 15) == 0)
			arg_ipc = 1;
		else if (strncmp(argv[i], "--cpu=", 6) == 0)
			read_cpu_list(argv[i] + 6);
		else if (strncmp(argv[i], "--nice=", 7) == 0) {
			cfg.nice = atoi(argv[i] + 7);
			if (getuid() != 0 &&cfg.nice < 0)
				cfg.nice = 0;
			arg_nice = 1;
		}
		else if (strncmp(argv[i], "--cgroup=", 9) == 0) {
			if (checkcfg(CFG_CGROUP)) {
				if (option_cgroup) {
					fprintf(stderr, "Error: only one cgroup can be defined\n");
					exit(1);
				}
				cfg.cgroup = strdup(argv[i] + 9);
				if (!cfg.cgroup)
					errExit("strdup");

				check_cgroup_file(cfg.cgroup);
				set_cgroup(cfg.cgroup, getpid());
				option_cgroup = 1;
			}
			else
				exit_err_feature("cgroup");
		}

		//*************************************
		// filesystem
		//*************************************
		else if (strcmp(argv[i], "--allusers") == 0)
			arg_allusers = 1;
		else if (strncmp(argv[i], "--bind=", 7) == 0) {
			if (checkcfg(CFG_BIND)) {
				char *line;
				if (asprintf(&line, "bind %s", argv[i] + 7) == -1)
					errExit("asprintf");

				profile_check_line(line, 0, NULL);	// will exit if something wrong
				profile_add(line);
			}
			else
				exit_err_feature("bind");
		}
		else if (strncmp(argv[i], "--tmpfs=", 8) == 0) {
			char *line;
			if (asprintf(&line, "tmpfs %s", argv[i] + 8) == -1)
				errExit("asprintf");

			profile_check_line(line, 0, NULL);	// will exit if something wrong
			profile_add(line);
		}

		else if (strncmp(argv[i], "--blacklist=", 12) == 0) {
			char *line;
			if (asprintf(&line, "blacklist %s", argv[i] + 12) == -1)
				errExit("asprintf");

			profile_check_line(line, 0, NULL);	// will exit if something wrong
			profile_add(line);
		}
		else if (strncmp(argv[i], "--noblacklist=", 14) == 0) {
			char *line;
			if (asprintf(&line, "noblacklist %s", argv[i] + 14) == -1)
				errExit("asprintf");

			profile_check_line(line, 0, NULL);	// will exit if something wrong
			profile_add(line);
		}
		else if (strncmp(argv[i], "--whitelist=", 12) == 0) {
			char *line;
			if (asprintf(&line, "whitelist %s", argv[i] + 12) == -1)
				errExit("asprintf");

			profile_check_line(line, 0, NULL);	// will exit if something wrong
			profile_add(line);
		}
		else if (strncmp(argv[i], "--nowhitelist=", 14) == 0) {
			char *line;
			if (asprintf(&line, "nowhitelist %s", argv[i] + 14) == -1)
				errExit("asprintf");

			profile_check_line(line, 0, NULL);	// will exit if something wrong
			profile_add(line);
		}

		else if (strncmp(argv[i], "--mkdir=", 8) == 0) {
			char *line;
			if (asprintf(&line, "mkdir %s", argv[i] + 8) == -1)
				errExit("asprintf");
			/* Note: Applied both immediately in profile_check_line()
			 *       and later on via fs_blacklist().
			 */
			profile_check_line(line, 0, NULL);
			profile_add(line);
		}
		else if (strncmp(argv[i], "--mkfile=", 9) == 0) {
			char *line;
			if (asprintf(&line, "mkfile %s", argv[i] + 9) == -1)
				errExit("asprintf");
			/* Note: Applied both immediately in profile_check_line()
			 *       and later on via fs_blacklist().
			 */
			profile_check_line(line, 0, NULL);
			profile_add(line);
		}
		else if (strncmp(argv[i], "--read-only=", 12) == 0) {
			char *line;
			if (asprintf(&line, "read-only %s", argv[i] + 12) == -1)
				errExit("asprintf");

			profile_check_line(line, 0, NULL);	// will exit if something wrong
			profile_add(line);
		}
		else if (strncmp(argv[i], "--noexec=", 9) == 0) {
			char *line;
			if (asprintf(&line, "noexec %s", argv[i] + 9) == -1)
				errExit("asprintf");

			profile_check_line(line, 0, NULL);	// will exit if something wrong
			profile_add(line);
		}
		else if (strncmp(argv[i], "--read-write=", 13) == 0) {
			char *line;
			if (asprintf(&line, "read-write %s", argv[i] + 13) == -1)
				errExit("asprintf");

			profile_check_line(line, 0, NULL);	// will exit if something wrong
			profile_add(line);
		}
		else if (strcmp(argv[i], "--disable-mnt") == 0)
			arg_disable_mnt = 1;
#ifdef HAVE_OVERLAYFS
		else if (strcmp(argv[i], "--overlay") == 0) {
			if (checkcfg(CFG_OVERLAYFS)) {
				if (arg_overlay) {
					fprintf(stderr, "Error: only one overlay command is allowed\n");
					exit(1);
				}

				if (cfg.chrootdir) {
					fprintf(stderr, "Error: --overlay and --chroot options are mutually exclusive\n");
					exit(1);
				}
				struct stat s;
				if (stat("/proc/sys/kernel/grsecurity", &s) == 0) {
					fprintf(stderr, "Error: --overlay option is not available on Grsecurity systems\n");
					exit(1);
				}
				arg_overlay = 1;
				arg_overlay_keep = 1;

				char *subdirname;
				if (asprintf(&subdirname, "%d", getpid()) == -1)
					errExit("asprintf");
				cfg.overlay_dir = fs_check_overlay_dir(subdirname, arg_overlay_reuse);

				free(subdirname);
			}
			else
				exit_err_feature("overlayfs");
		}
		else if (strncmp(argv[i], "--overlay-named=", 16) == 0) {
			if (checkcfg(CFG_OVERLAYFS)) {
				if (arg_overlay) {
					fprintf(stderr, "Error: only one overlay command is allowed\n");
					exit(1);
				}
				if (cfg.chrootdir) {
					fprintf(stderr, "Error: --overlay and --chroot options are mutually exclusive\n");
					exit(1);
				}
				struct stat s;
				if (stat("/proc/sys/kernel/grsecurity", &s) == 0) {
					fprintf(stderr, "Error: --overlay option is not available on Grsecurity systems\n");
					exit(1);
				}
				arg_overlay = 1;
				arg_overlay_keep = 1;
				arg_overlay_reuse = 1;

				char *subdirname = argv[i] + 16;
				if (*subdirname == '\0') {
					fprintf(stderr, "Error: invalid overlay option\n");
					exit(1);
				}

				// check name
				invalid_filename(subdirname, 0); // no globbing
				if (strstr(subdirname, "..") || strstr(subdirname, "/")) {
					fprintf(stderr, "Error: invalid overlay name\n");
					exit(1);
				}
				cfg.overlay_dir = fs_check_overlay_dir(subdirname, arg_overlay_reuse);
			}
			else
				exit_err_feature("overlayfs");
		}
		else if (strcmp(argv[i], "--overlay-tmpfs") == 0) {
			if (checkcfg(CFG_OVERLAYFS)) {
				if (arg_overlay) {
					fprintf(stderr, "Error: only one overlay command is allowed\n");
					exit(1);
				}
				if (cfg.chrootdir) {
					fprintf(stderr, "Error: --overlay and --chroot options are mutually exclusive\n");
					exit(1);
				}
				struct stat s;
				if (stat("/proc/sys/kernel/grsecurity", &s) == 0) {
					fprintf(stderr, "Error: --overlay option is not available on Grsecurity systems\n");
					exit(1);
				}
				arg_overlay = 1;
			}
			else
				exit_err_feature("overlayfs");
		}
#endif
#ifdef HAVE_FIRETUNNEL
		else if (strcmp(argv[i], "--tunnel") == 0) {
			// try to connect to the default client side of the tunnel
			// if this fails, try the default server side of the tunnel
			if (access("/run/firetunnel/ftc", R_OK) == 0)
				profile_read("/run/firetunnel/ftc");
			else if (access("/run/firetunnel/fts", R_OK) == 0)
				profile_read("/run/firetunnel/fts");
			else {
				fprintf(stderr, "Error: no default firetunnel found, please specify it using --tunnel=devname option\n");
				exit(1);
			}
		}
		else if (strncmp(argv[i], "--tunnel=", 9) == 0) {
			char *fname;

			if (asprintf(&fname, "/run/firetunnel/%s", argv[i] + 9) == -1)
				errExit("asprintf");
			invalid_filename(fname, 0); // no globbing
			if (access(fname, R_OK) == 0)
				profile_read(fname);
			else {
				fprintf(stderr, "Error: tunnel not found\n");
				exit(1);
			}
		}
#endif
		else if (strncmp(argv[i], "--include=", 10) == 0) {
			char *ppath = expand_macros(argv[i] + 10);
			if (!ppath)
				errExit("strdup");

			char *ptr = ppath;
			while (*ptr != '/' && *ptr != '\0')
				ptr++;
			if (*ptr == '\0') {
				if (access(ppath, R_OK)) {
					profile_read(ppath);
				}
				else {
					// ppath contains no '/' and is not a local file, assume it's a name
					int rv = profile_find_firejail(ppath, 0);
					if (!rv) {
						fprintf(stderr, "Error: no profile with name \"%s\" found.\n", ppath);
						exit(1);
					}
				}
			}
			else {
				// ppath contains a '/', assume it's a path
				profile_read(ppath);
			}

			free(ppath);
		}
		else if (strncmp(argv[i], "--profile=", 10) == 0) {
			// multiple profile files are allowed!

			if (arg_noprofile) {
				fprintf(stderr, "Error: --noprofile and --profile options are mutually exclusive\n");
				exit(1);
			}

			char *ppath = expand_macros(argv[i] + 10);
			if (!ppath)
				errExit("strdup");

			// checking for strange chars in the file name, no globbing
			invalid_filename(ppath, 0);

			if (*ppath == ':' || access(ppath, R_OK) || is_dir(ppath)) {
				int has_colon = (*ppath == ':');
				char *ptr = ppath;
				while (*ptr != '/' && *ptr != '.' && *ptr != '\0')
					ptr++;
				// profile path contains no / or . chars,
				// assume its a profile name
				if (*ptr != '\0') {
					fprintf(stderr, "Error: inaccessible profile file: %s\n", ppath);
					exit(1);
				}

				// profile was not read in previously, try to see if
				// we were given a profile name.
				if (!profile_find_firejail(ppath + has_colon, 1)) {
					// do not fall through to default profile,
					// because the user should be notified that
					// given profile arg could not be used.
					fprintf(stderr, "Error: no profile with name \"%s\" found.\n", ppath);
					exit(1);
				}
				else
					custom_profile = 1;
			}
			else {
				profile_read(ppath);
				custom_profile = 1;
			}
			free(ppath);
		}
		else if (strcmp(argv[i], "--noprofile") == 0) {
			if (custom_profile) {
				fprintf(stderr, "Error: --profile and --noprofile options are mutually exclusive\n");
				exit(1);
			}
			arg_noprofile = 1;
			// force keep-config-pulse in order to keep ~/.config/pulse as is
			arg_keep_config_pulse = 1;
		}
		else if (strncmp(argv[i], "--ignore=", 9) == 0) {
			if (custom_profile) {
				fprintf(stderr, "Error: please use --profile after --ignore\n");
				exit(1);
			}
			profile_add_ignore(argv[i] + 9);
		}
		else if (strncmp(argv[i], "--keep-fd=", 10) == 0) {
			if (strcmp(argv[i] + 10, "all") == 0)
				arg_keep_fd_all = 1;
			else {
				const char *add = argv[i] + 10;
				profile_list_augment(&cfg.keep_fd, add);
			}
		}
#ifdef HAVE_CHROOT
		else if (strncmp(argv[i], "--chroot=", 9) == 0) {
			if (checkcfg(CFG_CHROOT)) {
				if (arg_overlay) {
					fprintf(stderr, "Error: --overlay and --chroot options are mutually exclusive\n");
					exit(1);
				}

				struct stat s;
				if (stat("/proc/sys/kernel/grsecurity", &s) == 0) {
					fprintf(stderr, "Error: --chroot option is not available on Grsecurity systems\n");
					exit(1);
				}
				// extract chroot dirname
				cfg.chrootdir = argv[i] + 9;
				if (*cfg.chrootdir == '\0') {
					fprintf(stderr, "Error: invalid chroot option\n");
					exit(1);
				}
				invalid_filename(cfg.chrootdir, 0); // no globbing

				// if the directory starts with ~, expand the home directory
				if (*cfg.chrootdir == '~') {
					char *tmp;
					if (asprintf(&tmp, "%s%s", cfg.homedir, cfg.chrootdir + 1) == -1)
						errExit("asprintf");
					cfg.chrootdir = tmp;
				}
				// check chroot directory
				fs_check_chroot_dir();
			}
			else
				exit_err_feature("chroot");
		}
#endif
		else if (strcmp(argv[i], "--writable-etc") == 0) {
			if (cfg.etc_private_keep) {
				fprintf(stderr, "Error: --private-etc and --writable-etc are mutually exclusive\n");
				exit(1);
			}
			arg_writable_etc = 1;
		}
		else if (strcmp(argv[i], "--keep-config-pulse") == 0) {
			arg_keep_config_pulse = 1;
		}
		else if (strcmp(argv[i], "--writable-var") == 0) {
			arg_writable_var = 1;
		}
		else if (strcmp(argv[i], "--keep-var-tmp") == 0) {
		        arg_keep_var_tmp = 1;
		}
		else if (strcmp(argv[i], "--writable-run-user") == 0) {
			arg_writable_run_user = 1;
		}
		else if (strcmp(argv[i], "--writable-var-log") == 0) {
			arg_writable_var_log = 1;
		}
		else if (strcmp(argv[i], "--machine-id") == 0) {
			arg_machineid = 1;
		}
		else if (strcmp(argv[i], "--private") == 0) {
			arg_private = 1;
		}
		else if (strncmp(argv[i], "--private=", 10) == 0) {
			if (cfg.home_private_keep) {
				fprintf(stderr, "Error: a private list of files was already defined with --private-home option.\n");
				exit(1);
			}

			// extract private home dirname
			cfg.home_private = argv[i] + 10;
			if (*cfg.home_private == '\0') {
				fprintf(stderr, "Error: invalid private option\n");
				exit(1);
			}
			fs_check_private_dir();

			// downgrade to --private if the directory is the user home directory
			if (strcmp(cfg.home_private, cfg.homedir) == 0) {
				free(cfg.home_private);
				cfg.home_private = NULL;
			}
			arg_private = 1;
		}
#ifdef HAVE_PRIVATE_HOME
		else if (strncmp(argv[i], "--private-home=", 15) == 0) {
			if (checkcfg(CFG_PRIVATE_HOME)) {
				if (cfg.home_private) {
					fprintf(stderr, "Error: a private home directory was already defined with --private option.\n");
					exit(1);
				}

				// extract private home dirname
				if (*(argv[i] + 15) == '\0') {
					fprintf(stderr, "Error: invalid private-home option\n");
					exit(1);
				}
				if (cfg.home_private_keep) {
					if ( asprintf(&cfg.home_private_keep, "%s,%s", cfg.home_private_keep, argv[i] + 15) < 0 )
						errExit("asprintf");
				} else
					cfg.home_private_keep = argv[i] + 15;
				arg_private = 1;
			}
			else
				exit_err_feature("private-home");
		}
#endif
		else if (strcmp(argv[i], "--private-dev") == 0) {
			arg_private_dev = 1;
		}
		else if (strcmp(argv[i], "--keep-dev-shm") == 0) {
			arg_keep_dev_shm = 1;
		}
		else if (strncmp(argv[i], "--private-etc=", 14) == 0) {
			if (checkcfg(CFG_PRIVATE_ETC)) {
				if (arg_writable_etc) {
					fprintf(stderr, "Error: --private-etc and --writable-etc are mutually exclusive\n");
					exit(1);
				}

				// extract private etc list
				if (*(argv[i] + 14) == '\0') {
					fprintf(stderr, "Error: invalid private-etc option\n");
					exit(1);
				}
				if (cfg.etc_private_keep) {
					if ( asprintf(&cfg.etc_private_keep, "%s,%s", cfg.etc_private_keep, argv[i] + 14) < 0 )
						errExit("asprintf");
				} else
					cfg.etc_private_keep = argv[i] + 14;
				arg_private_etc = 1;
			}
			else
				exit_err_feature("private-etc");
		}
		else if (strncmp(argv[i], "--private-opt=", 14) == 0) {
			if (checkcfg(CFG_PRIVATE_OPT)) {
				// extract private opt list
				if (*(argv[i] + 14) == '\0') {
					fprintf(stderr, "Error: invalid private-opt option\n");
					exit(1);
				}
				if (cfg.opt_private_keep) {
					if ( asprintf(&cfg.opt_private_keep, "%s,%s", cfg.opt_private_keep, argv[i] + 14) < 0 )
						errExit("asprintf");
				} else
					cfg.opt_private_keep = argv[i] + 14;
				arg_private_opt = 1;
			}
			else
				exit_err_feature("private-opt");
		}
		else if (strncmp(argv[i], "--private-srv=", 14) == 0) {
			if (checkcfg(CFG_PRIVATE_SRV)) {
				// extract private srv list
				if (*(argv[i] + 14) == '\0') {
					fprintf(stderr, "Error: invalid private-srv option\n");
					exit(1);
				}
				if (cfg.srv_private_keep) {
					if ( asprintf(&cfg.srv_private_keep, "%s,%s", cfg.srv_private_keep, argv[i] + 14) < 0 )
						errExit("asprintf");
				} else
					cfg.srv_private_keep = argv[i] + 14;
				arg_private_srv = 1;
			}
			else
				exit_err_feature("private-srv");
		}
		else if (strncmp(argv[i], "--private-bin=", 14) == 0) {
			if (checkcfg(CFG_PRIVATE_BIN)) {
				// extract private bin list
				if (*(argv[i] + 14) == '\0') {
					fprintf(stderr, "Error: invalid private-bin option\n");
					exit(1);
				}
				if (cfg.bin_private_keep) {
					if ( asprintf(&cfg.bin_private_keep, "%s,%s", cfg.bin_private_keep, argv[i] + 14) < 0 )
						errExit("asprintf");
				} else
					cfg.bin_private_keep = argv[i] + 14;
				arg_private_bin = 1;
			}
			else
				exit_err_feature("private-bin");
		}
		else if (strncmp(argv[i], "--private-lib", 13) == 0) {
			if (checkcfg(CFG_PRIVATE_LIB)) {
				// extract private lib list (if any)
				if (argv[i][13] == '=') {
					if (cfg.lib_private_keep) {
						if (argv[i][14] != '\0' && asprintf(&cfg.lib_private_keep, "%s,%s", cfg.lib_private_keep, argv[i] + 14) < 0)
							errExit("asprintf");
					} else
						cfg.lib_private_keep = argv[i] + 14;
				}
				arg_private_lib = 1;
			}
			else
				exit_err_feature("private-lib");
		}
		else if (strcmp(argv[i], "--private-tmp") == 0) {
			arg_private_tmp = 1;
		}
#ifdef HAVE_USERTMPFS
		else if (strcmp(argv[i], "--private-cache") == 0) {
			if (checkcfg(CFG_PRIVATE_CACHE))
				arg_private_cache = 1;
			else
				exit_err_feature("private-cache");
		}
#endif
		else if (strcmp(argv[i], "--private-cwd") == 0) {
			cfg.cwd = NULL;
			arg_private_cwd = 1;
		}
		else if (strncmp(argv[i], "--private-cwd=", 14) == 0) {
			if (*(argv[i] + 14) == '\0') {
				fprintf(stderr, "Error: invalid private-cwd option\n");
				exit(1);
			}

			fs_check_private_cwd(argv[i] + 14);
			arg_private_cwd = 1;
		}

		//*************************************
		// hostname, etc
		//*************************************
		else if (strncmp(argv[i], "--name=", 7) == 0) {
			cfg.name = argv[i] + 7;
			if (strlen(cfg.name) == 0) {
				fprintf(stderr, "Error: please provide a name for sandbox\n");
				return 1;
			}
		}
		else if (strncmp(argv[i], "--hostname=", 11) == 0) {
			cfg.hostname = argv[i] + 11;
			if (strlen(cfg.hostname) == 0) {
				fprintf(stderr, "Error: please provide a hostname for sandbox\n");
				return 1;
			}
		}
		else if (strcmp(argv[i], "--nogroups") == 0)
			arg_nogroups = 1;
#ifdef HAVE_USERNS
		else if (strcmp(argv[i], "--noroot") == 0) {
			if (checkcfg(CFG_USERNS))
				check_user_namespace();
			else
				exit_err_feature("noroot");
		}
#endif
		else if (strcmp(argv[i], "--nonewprivs") == 0)
			arg_nonewprivs = 1;
		else if (strncmp(argv[i], "--env=", 6) == 0)
			env_store(argv[i] + 6, SETENV);
		else if (strncmp(argv[i], "--rmenv=", 8) == 0)
			env_store(argv[i] + 8, RMENV);
		else if (strcmp(argv[i], "--nosound") == 0)
			arg_nosound = 1;
		else if (strcmp(argv[i], "--noautopulse") == 0)
			arg_keep_config_pulse = 1;
		else if (strcmp(argv[i], "--novideo") == 0)
			arg_novideo = 1;
		else if (strcmp(argv[i], "--no3d") == 0)
			arg_no3d = 1;
		else if (strcmp(argv[i], "--noprinters") == 0) {
			arg_noprinters = 1;
			profile_add("blacklist /dev/lp*");
			profile_add("blacklist /run/cups/cups.sock");
		}
		else if (strcmp(argv[i], "--notv") == 0)
			arg_notv = 1;
		else if (strcmp(argv[i], "--nodvd") == 0)
			arg_nodvd = 1;
		else if (strcmp(argv[i], "--nou2f") == 0)
			arg_nou2f = 1;
		else if (strcmp(argv[i], "--noinput") == 0)
			arg_noinput = 1;
		else if (strcmp(argv[i], "--nodbus") == 0) {
			arg_dbus_user = DBUS_POLICY_BLOCK;
			arg_dbus_system = DBUS_POLICY_BLOCK;
		}

		//*************************************
		// D-BUS proxy
		//*************************************
#ifdef HAVE_DBUSPROXY
		else if (strncmp("--dbus-user=", argv[i], 12) == 0) {
			if (strcmp("filter", argv[i] + 12) == 0) {
				if (arg_dbus_user == DBUS_POLICY_BLOCK) {
					fprintf(stderr, "Warning: Cannot relax --dbus-user policy, it is already set to block\n");
				} else {
					arg_dbus_user = DBUS_POLICY_FILTER;
				}
			} else if (strcmp("none", argv[i] + 12) == 0) {
				if (arg_dbus_log_user) {
					fprintf(stderr, "Error: --dbus-user.log requires --dbus-user=filter\n");
					exit(1);
				}
				arg_dbus_user = DBUS_POLICY_BLOCK;
			} else {
				fprintf(stderr, "Unknown dbus-user policy: %s\n", argv[i] + 12);
				exit(1);
			}
		}
		else if (strncmp(argv[i], "--dbus-user.see=", 16) == 0) {
			char *line;
			if (asprintf(&line, "dbus-user.see %s", argv[i] + 16) == -1)
				errExit("asprintf");

			profile_check_line(line, 0, NULL); // will exit if something wrong
			profile_add(line);
		}
		else if (strncmp(argv[i], "--dbus-user.talk=", 17) == 0) {
			char *line;
			if (asprintf(&line, "dbus-user.talk %s", argv[i] + 17) == -1)
				errExit("asprintf");

			profile_check_line(line, 0, NULL); // will exit if something wrong
			profile_add(line);
		}
		else if (strncmp(argv[i], "--dbus-user.own=", 16) == 0) {
			char *line;
			if (asprintf(&line, "dbus-user.own %s", argv[i] + 16) == -1)
				errExit("asprintf");

			profile_check_line(line, 0, NULL); // will exit if something wrong
			profile_add(line);
		}
		else if (strncmp(argv[i], "--dbus-user.call=", 17) == 0) {
			char *line;
			if (asprintf(&line, "dbus-user.call %s", argv[i] + 17) == -1)
				errExit("asprintf");

			profile_check_line(line, 0, NULL); // will exit if something wrong
			profile_add(line);
		}
		else if (strncmp(argv[i], "--dbus-user.broadcast=", 22) == 0) {
			char *line;
			if (asprintf(&line, "dbus-user.broadcast %s", argv[i] + 22) == -1)
				errExit("asprintf");

			profile_check_line(line, 0, NULL); // will exit if something wrong
			profile_add(line);
		}
		else if (strncmp("--dbus-system=", argv[i], 14) == 0) {
			if (strcmp("filter", argv[i] + 14) == 0) {
				if (arg_dbus_system == DBUS_POLICY_BLOCK) {
					fprintf(stderr, "Warning: Cannot relax --dbus-system policy, it is already set to block\n");
				} else {
					arg_dbus_system = DBUS_POLICY_FILTER;
				}
			} else if (strcmp("none", argv[i] + 14) == 0) {
				if (arg_dbus_log_system) {
					fprintf(stderr, "Error: --dbus-system.log requires --dbus-system=filter\n");
					exit(1);
				}
				arg_dbus_system = DBUS_POLICY_BLOCK;
			} else {
				fprintf(stderr, "Unknown dbus-system policy: %s\n", argv[i] + 14);
				exit(1);
			}
		}
		else if (strncmp(argv[i], "--dbus-system.see=", 18) == 0) {
			char *line;
			if (asprintf(&line, "dbus-system.see %s", argv[i] + 18) == -1)
				errExit("asprintf");

			profile_check_line(line, 0, NULL); // will exit if something wrong
			profile_add(line);
		}
		else if (strncmp(argv[i], "--dbus-system.talk=", 19) == 0) {
			char *line;
			if (asprintf(&line, "dbus-system.talk %s", argv[i] + 19) == -1)
				errExit("asprintf");

			profile_check_line(line, 0, NULL); // will exit if something wrong
			profile_add(line);
		}
		else if (strncmp(argv[i], "--dbus-system.own=", 18) == 0) {
			char *line;
			if (asprintf(&line, "dbus-system.own %s", argv[i] + 18) == -1)
				errExit("asprintf");

			profile_check_line(line, 0, NULL); // will exit if something wrong
			profile_add(line);
		}
		else if (strncmp(argv[i], "--dbus-system.call=", 19) == 0) {
			char *line;
			if (asprintf(&line, "dbus-system.call %s", argv[i] + 19) == -1)
				errExit("asprintf");

			profile_check_line(line, 0, NULL); // will exit if something wrong
			profile_add(line);
		}
		else if (strncmp(argv[i], "--dbus-system.broadcast=", 24) == 0) {
			char *line;
			if (asprintf(&line, "dbus-system.broadcast %s", argv[i] + 24) == -1)
				errExit("asprintf");

			profile_check_line(line, 0, NULL); // will exit if something wrong
			profile_add(line);
		}
		else if (strncmp(argv[i], "--dbus-log=", 11) == 0) {
			if (arg_dbus_log_file != NULL) {
				fprintf(stderr, "Error: --dbus-log option already specified\n");
				exit(1);
			}
			arg_dbus_log_file = argv[i] + 11;
		}
		else if (strcmp(argv[i], "--dbus-user.log") == 0) {
			if (arg_dbus_user != DBUS_POLICY_FILTER) {
				fprintf(stderr, "Error: --dbus-user.log requires --dbus-user=filter\n");
				exit(1);
			}
			arg_dbus_log_user = 1;
		}
		else if (strcmp(argv[i], "--dbus-system.log") == 0) {
			if (arg_dbus_system != DBUS_POLICY_FILTER) {
				fprintf(stderr, "Error: --dbus-system.log requires --dbus-system=filter\n");
				exit(1);
			}
			arg_dbus_log_system = 1;
		}
#endif

		//*************************************
		// network
		//*************************************
		else if (strcmp(argv[i], "--net=none") == 0) {
			arg_nonetwork  = 1;
			cfg.bridge0.configured = 0;
			cfg.bridge1.configured = 0;
			cfg.bridge2.configured = 0;
			cfg.bridge3.configured = 0;
			cfg.interface0.configured = 0;
			cfg.interface1.configured = 0;
			cfg.interface2.configured = 0;
			cfg.interface3.configured = 0;
			continue;
		}
#ifdef HAVE_NETWORK
		else if (strcmp(argv[i], "--netlock") == 0) {
			if (checkcfg(CFG_NETWORK))
				arg_netlock = 1;
			else
				exit_err_feature("networking");
		}
		else if (strncmp(argv[i], "--netlock=", 10) == 0) {
			if (checkcfg(CFG_NETWORK)) {
				pid_t pid = require_pid(argv[i] + 10);
				netfilter_netlock(pid);
			}
			else
				exit_err_feature("networking");
			exit(0);
		}
		else if (strncmp(argv[i], "--interface=", 12) == 0) {
			if (checkcfg(CFG_NETWORK)) {
				// checks
				if (arg_nonetwork) {
					fprintf(stderr, "Error: --network=none and --interface are incompatible\n");
					exit(1);
				}
				if (strcmp(argv[i] + 12, "lo") == 0) {
					fprintf(stderr, "Error: cannot use lo device in --interface command\n");
					exit(1);
				}
				int ifindex = if_nametoindex(argv[i] + 12);
				if (ifindex <= 0) {
					fprintf(stderr, "Error: cannot find interface %s\n", argv[i] + 12);
					exit(1);
				}

				Interface *intf;
				if (cfg.interface0.configured == 0)
					intf = &cfg.interface0;
				else if (cfg.interface1.configured == 0)
					intf = &cfg.interface1;
				else if (cfg.interface2.configured == 0)
					intf = &cfg.interface2;
				else if (cfg.interface3.configured == 0)
					intf = &cfg.interface3;
				else {
					fprintf(stderr, "Error: maximum 4 interfaces are allowed\n");
					return 1;
				}

				intf->dev = strdup(argv[i] + 12);
				if (!intf->dev)
					errExit("strdup");

				if (net_get_if_addr(intf->dev, &intf->ip, &intf->mask, intf->mac, &intf->mtu)) {
					fwarning("interface %s is not configured\n", intf->dev);
				}
				intf->configured = 1;
			}
			else
				exit_err_feature("networking");
		}

		else if (strncmp(argv[i], "--net=", 6) == 0) {
			if (checkcfg(CFG_NETWORK)) {
				if (strcmp(argv[i] + 6, "none") == 0) {
					arg_nonetwork  = 1;
					cfg.bridge0.configured = 0;
					cfg.bridge1.configured = 0;
					cfg.bridge2.configured = 0;
					cfg.bridge3.configured = 0;
					cfg.interface0.configured = 0;
					cfg.interface1.configured = 0;
					cfg.interface2.configured = 0;
					cfg.interface3.configured = 0;
					continue;
				}

				if (strcmp(argv[i] + 6, "lo") == 0) {
					fprintf(stderr, "Error: cannot attach to lo device\n");
					exit(1);
				}

				Bridge *br;
				if (cfg.bridge0.configured == 0)
					br = &cfg.bridge0;
				else if (cfg.bridge1.configured == 0)
					br = &cfg.bridge1;
				else if (cfg.bridge2.configured == 0)
					br = &cfg.bridge2;
				else if (cfg.bridge3.configured == 0)
					br = &cfg.bridge3;
				else {
					fprintf(stderr, "Error: maximum 4 network devices are allowed\n");
					return 1;
				}
				br->dev = argv[i] + 6;
				br->configured = 1;
			}
			else
				exit_err_feature("networking");
		}

		else if (strncmp(argv[i], "--veth-name=", 12) == 0) {
			if (checkcfg(CFG_NETWORK)) {
				Bridge *br = last_bridge_configured();
				if (br == NULL) {
					fprintf(stderr, "Error: no network device configured\n");
					exit(1);
				}
				br->veth_name = strdup(argv[i] + 12);
				if (br->veth_name == NULL)
					errExit("strdup");
				if (*br->veth_name == '\0') {
					fprintf(stderr, "Error: no veth-name configured\n");
					exit(1);
				}
			}
			else
				exit_err_feature("networking");
		}

		else if (strcmp(argv[i], "--scan") == 0) {
			if (checkcfg(CFG_NETWORK)) {
				arg_scan = 1;
			}
			else
				exit_err_feature("networking");
		}
		else if (strncmp(argv[i], "--iprange=", 10) == 0) {
			if (checkcfg(CFG_NETWORK)) {
				Bridge *br = last_bridge_configured();
				if (br == NULL) {
					fprintf(stderr, "Error: no network device configured\n");
					return 1;
				}
				if (br->iprange_start || br->iprange_end) {
					fprintf(stderr, "Error: cannot configure the IP range twice for the same interface\n");
					return 1;
				}

				// parse option arguments
				char *firstip = argv[i] + 10;
				char *secondip = firstip;
				while (*secondip != '\0') {
					if (*secondip == ',')
						break;
					secondip++;
				}
				if (*secondip == '\0') {
					fprintf(stderr, "Error: invalid IP range\n");
					return 1;
				}
				*secondip = '\0';
				secondip++;

				// check addresses
				if (atoip(firstip, &br->iprange_start) || atoip(secondip, &br->iprange_end) ||
				    br->iprange_start >= br->iprange_end) {
					fprintf(stderr, "Error: invalid IP range\n");
					return 1;
				}
			}
			else
				exit_err_feature("networking");
		}

		else if (strncmp(argv[i], "--mac=", 6) == 0) {
			if (checkcfg(CFG_NETWORK)) {
				Bridge *br = last_bridge_configured();
				if (br == NULL) {
					fprintf(stderr, "Error: no network device configured\n");
					exit(1);
				}
				if (mac_not_zero(br->macsandbox)) {
					fprintf(stderr, "Error: cannot configure the MAC address twice for the same interface\n");
					exit(1);
				}

				// read the address
				if (atomac(argv[i] + 6, br->macsandbox)) {
					fprintf(stderr, "Error: invalid MAC address\n");
					exit(1);
				}

				// check multicast address
				if (br->macsandbox[0] & 1) {
					fprintf(stderr, "Error: invalid MAC address (multicast)\n");
					exit(1);
				}

			}
			else
				exit_err_feature("networking");
		}

		else if (strncmp(argv[i], "--mtu=", 6) == 0) {
			if (checkcfg(CFG_NETWORK)) {
				Bridge *br = last_bridge_configured();
				if (br == NULL) {
					fprintf(stderr, "Error: no network device configured\n");
					exit(1);
				}

				if (sscanf(argv[i] + 6, "%d", &br->mtu) != 1 || br->mtu < 576 || br->mtu > 9198) {
					fprintf(stderr, "Error: invalid mtu value\n");
					exit(1);
				}
			}
			else
				exit_err_feature("networking");
		}

		else if (strncmp(argv[i], "--ip=", 5) == 0) {
			if (checkcfg(CFG_NETWORK)) {
				Bridge *br = last_bridge_configured();
				if (br == NULL) {
					fprintf(stderr, "Error: no network device configured\n");
					exit(1);
				}
				if (br->arg_ip_none || br->ipsandbox) {
					fprintf(stderr, "Error: cannot configure the IP address twice for the same interface\n");
					exit(1);
				}

				// configure this IP address for the last bridge defined
				if (strcmp(argv[i] + 5, "none") == 0)
					br->arg_ip_none = 1;
				else if (strcmp(argv[i] + 5, "dhcp") == 0) {
					br->arg_ip_none = 1;
					br->arg_ip_dhcp = 1;
				} else {
					if (atoip(argv[i] + 5, &br->ipsandbox)) {
						fprintf(stderr, "Error: invalid IP address\n");
						exit(1);
					}
				}
			}
			else
				exit_err_feature("networking");
		}

		else if (strncmp(argv[i], "--netmask=", 10) == 0) {
			if (checkcfg(CFG_NETWORK)) {
				Bridge *br = last_bridge_configured();
				if (br == NULL) {
					fprintf(stderr, "Error: no network device configured\n");
					exit(1);
				}
				if (br->arg_ip_none || br->mask) {
					fprintf(stderr, "Error: cannot configure the network mask twice for the same interface\n");
					exit(1);
				}

				// configure this network mask for the last bridge defined
				if (atoip(argv[i] + 10, &br->mask)) {
					fprintf(stderr, "Error: invalid  network mask\n");
					exit(1);
				}
			}
			else
				exit_err_feature("networking");
		}

		else if (strncmp(argv[i], "--ip6=", 6) == 0) {
			if (checkcfg(CFG_NETWORK)) {
				Bridge *br = last_bridge_configured();
				if (br == NULL) {
					fprintf(stderr, "Error: no network device configured\n");
					exit(1);
				}
				if (br->arg_ip6_dhcp || br->ip6sandbox) {
					fprintf(stderr, "Error: cannot configure the IP address twice for the same interface\n");
					exit(1);
				}

				// configure this IP address for the last bridge defined
				if (strcmp(argv[i] + 6, "dhcp") == 0)
					br->arg_ip6_dhcp = 1;
				else {
					if (check_ip46_address(argv[i] + 6) == 0) {
						fprintf(stderr, "Error: invalid IPv6 address\n");
						exit(1);
					}

					br->ip6sandbox = strdup(argv[i] + 6);
					if (br->ip6sandbox == NULL)
						errExit("strdup");
				}
			}
			else
				exit_err_feature("networking");
		}


		else if (strncmp(argv[i], "--defaultgw=", 12) == 0) {
			if (checkcfg(CFG_NETWORK)) {
				if (atoip(argv[i] + 12, &cfg.defaultgw)) {
					fprintf(stderr, "Error: invalid IP address\n");
					exit(1);
				}
			}
			else
				exit_err_feature("networking");
		}
#endif
		else if (strncmp(argv[i], "--dns=", 6) == 0) {
			if (check_ip46_address(argv[i] + 6) == 0) {
				fprintf(stderr, "Error: invalid DNS server IPv4 or IPv6 address\n");
				exit(1);
			}
			char *dns = strdup(argv[i] + 6);
			if (!dns)
				errExit("strdup");

			if (cfg.dns1 == NULL)
				cfg.dns1 = dns;
			else if (cfg.dns2 == NULL)
				cfg.dns2 = dns;
			else if (cfg.dns3 == NULL)
				cfg.dns3 = dns;
			else if (cfg.dns4 == NULL)
				cfg.dns4 = dns;
			else {
				fwarning("up to 4 DNS servers can be specified, %s ignored\n", dns);
				free(dns);
			}
		}

		else if (strncmp(argv[i], "--hosts-file=", 13) == 0)
			cfg.hosts_file = fs_check_hosts_file(argv[i] + 13);

#ifdef HAVE_NETWORK
		else if (strcmp(argv[i], "--netfilter") == 0) {
			if (checkcfg(CFG_NETWORK)) {
				arg_netfilter = 1;
			}
			else
				exit_err_feature("networking");
		}

		else if (strncmp(argv[i], "--netfilter=", 12) == 0) {
			if (checkcfg(CFG_NETWORK)) {
				arg_netfilter = 1;
				arg_netfilter_file = argv[i] + 12;

				// expand tilde
				if (*arg_netfilter_file == '~') {
					char *tmp;
					if (asprintf(&tmp, "%s%s", cfg.homedir, arg_netfilter_file + 1) == -1)
						errExit("asprintf");
					arg_netfilter_file = tmp;
				}

				check_netfilter_file(arg_netfilter_file);
			}
			else
				exit_err_feature("networking");
		}

		else if (strncmp(argv[i], "--netfilter6=", 13) == 0) {
			if (checkcfg(CFG_NETWORK)) {
				arg_netfilter6 = 1;
				arg_netfilter6_file = argv[i] + 13;

				// expand tilde
				if (*arg_netfilter6_file == '~') {
					char *tmp;
					if (asprintf(&tmp, "%s%s", cfg.homedir, arg_netfilter6_file + 1) == -1)
						errExit("asprintf");
					arg_netfilter6_file = tmp;
				}

				check_netfilter_file(arg_netfilter6_file);
			}
			else
				exit_err_feature("networking");
		}

		else if (strncmp(argv[i], "--netns=", 8) == 0) {
			if (checkcfg(CFG_NETWORK)) {
				arg_netns = argv[i] + 8;
				check_netns(arg_netns);
			}
			else
				exit_err_feature("networking");
		}
#endif
		//*************************************
		// command
		//*************************************
		else if (strncmp(argv[i], "--timeout=", 10) == 0)
			cfg.timeout = extract_timeout(argv[i] + 10);
		else if (strcmp(argv[i], "--appimage") == 0) {
			// already handled
		}
		else if (strncmp(argv[i], "--oom=", 6) == 0) {
			// already handled
		}
		else if (strcmp(argv[i], "--shell=none") == 0) {
			arg_shell_none = 1;
			if (cfg.shell) {
				fprintf(stderr, "Error: a shell was already specified\n");
				return 1;
			}
		}
		else if (strncmp(argv[i], "--shell=", 8) == 0) {
			if (arg_shell_none) {
				fprintf(stderr, "Error: --shell=none was already specified.\n");
				return 1;
			}
			invalid_filename(argv[i] + 8, 0); // no globbing

			if (cfg.shell) {
				fprintf(stderr, "Error: only one user shell can be specified\n");
				return 1;
			}
			cfg.shell = argv[i] + 8;

			if (is_dir(cfg.shell) || strstr(cfg.shell, "..")) {
				fprintf(stderr, "Error: invalid shell\n");
				exit(1);
			}

			// access call checks as real UID/GID, not as effective UID/GID
			if(cfg.chrootdir) {
				char *shellpath;
				if (asprintf(&shellpath, "%s%s", cfg.chrootdir, cfg.shell) == -1)
					errExit("asprintf");
				if (access(shellpath, X_OK)) {
					fprintf(stderr, "Error: cannot access shell file in chroot\n");
					exit(1);
				}
				free(shellpath);
			} else if (access(cfg.shell, X_OK)) {
				fprintf(stderr, "Error: cannot access shell file\n");
				exit(1);
			}
		}
		else if (strcmp(argv[i], "-c") == 0) {
			arg_command = 1;
			if (i == (argc -  1)) {
				fprintf(stderr, "Error: option -c requires an argument\n");
				return 1;
			}
		}

		// unlike all other x11 features, this is available always
		else if (strcmp(argv[i], "--x11=none") == 0) {
			arg_x11_block = 1;
		}
#ifdef HAVE_X11
		else if (strcmp(argv[i], "--x11=xorg") == 0) {
			if (checkcfg(CFG_X11))
				arg_x11_xorg = 1;
			else
				exit_err_feature("x11");
		}
#endif
		else if (strncmp(argv[i], "--join-or-start=", 16) == 0) {
			// NOTE: this is second part of option handler,
			//		 atempt to find and join sandbox is done in other one

			// set sandbox name and start normally
			cfg.name = argv[i] + 16;
			if (strlen(cfg.name) == 0) {
				fprintf(stderr, "Error: please provide a name for sandbox\n");
				return 1;
			}
		}
		else if (strcmp(argv[i], "--deterministic-exit-code") == 0) {
			arg_deterministic_exit_code = 1;
		}
		else if (strcmp(argv[i], "--deterministic-shutdown") == 0) {
			arg_deterministic_shutdown = 1;
		}
		else if (strcmp(argv[i], "--tab") == 0)
			arg_tab = 1;
		else {
			// double dash - positional params to follow
			if (strcmp(argv[i], "--") == 0) {
				arg_doubledash = 1;
				i++;
				if (i  >= argc) {
					fprintf(stderr, "Error: program name not found\n");
					exit(1);
				}
			}
			// is this an invalid option?
			else if (*argv[i] == '-') {
				fprintf(stderr, "Error: invalid %s command line option\n", argv[i]);
				return 1;
			}

			// we have a program name coming
			if (arg_appimage) {
				cfg.command_name = strdup(argv[i]);
				if (!cfg.command_name)
					errExit("strdup");

				// disable shell=* for appimages
				arg_shell_none = 0;
			}
			else
				extract_command_name(i, argv);
			prog_index = i;
			break;
		}
	}
	EUID_ASSERT();

	// exit chroot, overlay and appimage sandboxes when caps are explicitly specified on command line
	if (getuid() != 0 && arg_caps_cmdline) {
		char *opt = NULL;
		if (arg_appimage)
			opt = "appimage";
		else if (arg_overlay)
			opt = "overlay";
		else if (cfg.chrootdir)
			opt = "chroot";

		if (opt) {
			fprintf(stderr, "Error: all capabilities are dropped for %s by default.\n"
				"Please remove --caps options from the command line.\n", opt);
			exit(1);
		}
	}

	// prog_index could still be -1 if no program was specified
	if (prog_index == -1 && arg_shell_none) {
		fprintf(stderr, "Error: shell=none configured, but no program specified\n");
		exit(1);
	}

	// check trace configuration
	if (arg_trace && arg_tracelog) {
		fwarning("--trace and --tracelog are mutually exclusive; --tracelog disabled\n");
	}

	// check user namespace (--noroot) options
	if (arg_noroot) {
		if (arg_overlay) {
			fwarning("--overlay and --noroot are mutually exclusive, --noroot disabled...\n");
			arg_noroot = 0;
		}
		else if (cfg.chrootdir) {
			fwarning("--chroot and --noroot are mutually exclusive, --noroot disabled...\n");
			arg_noroot = 0;
		}
	}

	// check writable_etc and DNS/DHCP
	if (arg_writable_etc) {
		if (cfg.dns1 != NULL || any_dhcp()) {
			// we could end up overwriting the real /etc/resolv.conf, so we better exit now!
			fprintf(stderr, "Error: --dns/--ip=dhcp and --writable-etc are mutually exclusive\n");
			exit(1);
		}
	}



	// enable seccomp if only seccomp.block-secondary was specified
	if (arg_seccomp_block_secondary)
		arg_seccomp = 1;

	// log command
	logargs(argc, argv);
	if (fullargc) {
		char *msg;
		if (asprintf(&msg, "user %s entering restricted shell", cfg.username) == -1)
			errExit("asprintf");
		logmsg(msg);
		free(msg);
	}

	// guess shell if unspecified
	if (!arg_shell_none && !cfg.shell) {
		cfg.shell = guess_shell();
		if (!cfg.shell) {
			fprintf(stderr, "Error: unable to guess your shell, please set explicitly by using --shell option.\n");
			exit(1);
		}
		if (arg_debug)
			printf("Autoselecting %s as shell\n", cfg.shell);
	}

	// build the sandbox command
	if (prog_index == -1 && cfg.shell) {
		assert(cfg.command_line == NULL); // runs cfg.shell
		if (arg_appimage) {
			fprintf(stderr, "Error: no appimage archive specified\n");
			exit(1);
		}

		cfg.window_title = cfg.shell;
		cfg.command_name = cfg.shell;
	}
	else if (arg_appimage) {
		if (arg_debug)
			printf("Configuring appimage environment\n");
		appimage_set(cfg.command_name);
		build_appimage_cmdline(&cfg.command_line, &cfg.window_title, argc, argv, prog_index, true);
	}
	else {
		// Only add extra quotes if we were not launched by sshd.
		build_cmdline(&cfg.command_line, &cfg.window_title, argc, argv, prog_index, !parent_sshd);
	}
/*	else {
		fprintf(stderr, "Error: command must be specified when --shell=none used.\n");
		exit(1);
	}*/

	assert(cfg.command_name);
	if (arg_debug)
		printf("Command name #%s#\n", cfg.command_name);


	// load the profile
	if (!arg_noprofile && !custom_profile) {
		if (arg_appimage) {
			custom_profile = appimage_find_profile(cfg.command_name);
			// disable shell=* for appimages
			arg_shell_none = 0;
		}
		else
			custom_profile = profile_find_firejail(cfg.command_name, 1);
	}

	// use default.profile as the default
	if (!custom_profile && !arg_noprofile) {
		char *profile_name = DEFAULT_USER_PROFILE;
		if (getuid() == 0)
			profile_name = DEFAULT_ROOT_PROFILE;
		if (arg_debug)
			printf("Attempting to find %s.profile...\n", profile_name);

		custom_profile = profile_find_firejail(profile_name, 1);

		if (!custom_profile) {
			fprintf(stderr, "Error: no %s installed\n", profile_name);
			exit(1);
		}

		if (custom_profile)
			fmessage("\n** Note: you can use --noprofile to disable %s.profile **\n\n", profile_name);
	}
	EUID_ASSERT();

	// block X11 sockets
	if (arg_x11_block)
		x11_block();

	// check network configuration options - it will exit if anything went wrong
	net_check_cfg();

	// customization of default seccomp filter
	if (config_seccomp_filter_add) {
		if (arg_seccomp && !cfg.seccomp_list_keep && !cfg.seccomp_list_drop)
			profile_list_augment(&cfg.seccomp_list, config_seccomp_filter_add);

		if (arg_seccomp32 && !cfg.seccomp_list_keep32 && !cfg.seccomp_list_drop32)
			profile_list_augment(&cfg.seccomp_list32, config_seccomp_filter_add);
	}

	if (arg_seccomp)
		arg_seccomp_postexec = check_postexec(cfg.seccomp_list) || check_postexec(cfg.seccomp_list_drop);

	bool need_preload = arg_trace || arg_tracelog || arg_seccomp_postexec;
	if (need_preload && (cfg.seccomp_list32 || cfg.seccomp_list_drop32 || cfg.seccomp_list_keep32))
		fwarning("preload libraries (trace, tracelog, postexecseccomp due to seccomp.drop=execve etc.) are incompatible with 32 bit filters\n");

	// check and assign an IP address - for macvlan it will be done again in the sandbox!
	if (any_bridge_configured()) {
		EUID_ROOT();
		lockfd_network = open(RUN_NETWORK_LOCK_FILE, O_WRONLY | O_CREAT | O_CLOEXEC, S_IRUSR | S_IWUSR);
		if (lockfd_network != -1) {
			int rv = fchown(lockfd_network, 0, 0);
			(void) rv;
			flock(lockfd_network, LOCK_EX);
		}

		if (cfg.bridge0.configured && cfg.bridge0.arg_ip_none == 0)
			check_network(&cfg.bridge0);
		if (cfg.bridge1.configured && cfg.bridge1.arg_ip_none == 0)
			check_network(&cfg.bridge1);
		if (cfg.bridge2.configured && cfg.bridge2.arg_ip_none == 0)
			check_network(&cfg.bridge2);
		if (cfg.bridge3.configured && cfg.bridge3.arg_ip_none == 0)
			check_network(&cfg.bridge3);

		// save network mapping in shared memory
		network_set_run_file(sandbox_pid);
		EUID_USER();
	}
	EUID_ASSERT();

	if (arg_noroot && arg_overlay) {
		fwarning("--overlay and --noroot are mutually exclusive, noroot disabled\n");
		arg_noroot = 0;
	}
	else if (arg_noroot && cfg.chrootdir) {
		fwarning("--chroot and --noroot are mutually exclusive, noroot disabled\n");
		arg_noroot = 0;
	}


	// set name and x11 run files
	EUID_ROOT();
	lockfd_directory = open(RUN_DIRECTORY_LOCK_FILE, O_WRONLY | O_CREAT | O_CLOEXEC, S_IRUSR | S_IWUSR);
	if (lockfd_directory != -1) {
		int rv = fchown(lockfd_directory, 0, 0);
		(void) rv;
		flock(lockfd_directory, LOCK_EX);
	}
	if (cfg.name)
		set_name_run_file(sandbox_pid);
	int display = x11_display();
	if (display > 0)
		set_x11_run_file(sandbox_pid, display);
	if (lockfd_directory != -1) {
		flock(lockfd_directory, LOCK_UN);
		close(lockfd_directory);
	}
	EUID_USER();

#ifdef HAVE_DBUSPROXY
	if (checkcfg(CFG_DBUS)) {
		dbus_check_profile();
		if (arg_dbus_user == DBUS_POLICY_FILTER ||
			arg_dbus_system == DBUS_POLICY_FILTER) {
			EUID_ROOT();
			dbus_proxy_start();
			EUID_USER();
		}
	}
#endif

	// create the parent-child communication pipe
	if (pipe2(parent_to_child_fds, O_CLOEXEC) < 0)
		errExit("pipe");
	if (pipe2(child_to_parent_fds, O_CLOEXEC) < 0)
		errExit("pipe");

	// clone environment
	int flags = CLONE_NEWNS | CLONE_NEWPID | CLONE_NEWUTS | SIGCHLD;

	// in root mode also enable CLONE_NEWIPC
	// in user mode CLONE_NEWIPC will break MIT Shared Memory Extension (MIT-SHM)
	if (getuid() == 0 || arg_ipc) {
		flags |= CLONE_NEWIPC;
		if (arg_debug)
			printf("Enabling IPC namespace\n");
	}

	if (any_bridge_configured() || any_interface_configured() || arg_nonetwork) {
		flags |= CLONE_NEWNET;
	}
	else if (arg_debug)
		printf("Using the local network stack\n");

	EUID_ASSERT();
	EUID_ROOT();
#ifdef __ia64__
	child = __clone2(sandbox,
		child_stack,
		STACK_SIZE,
		flags,
		NULL);
#else
	child = clone(sandbox,
		child_stack + STACK_SIZE,
		flags,
		NULL);
#endif
	if (child == -1)
		errExit("clone");
	EUID_USER();

	if (!arg_command && !arg_quiet) {
		fmessage("Parent pid %u, child pid %u\n", sandbox_pid, child);
		// print the path of the new log directory
		if (getuid() == 0) // only for root
			printf("The new log directory is /proc/%d/root/var/log\n", child);
	}

	if (!arg_nonetwork) {
		EUID_ROOT();
		pid_t net_child = fork();
		if (net_child < 0)
			errExit("fork");
		if (net_child == 0) {
			// elevate privileges in order to get grsecurity working
			if (setreuid(0, 0))
				errExit("setreuid");
			if (setregid(0, 0))
				errExit("setregid");
			network_main(child);
			if (arg_debug)
				printf("Host network configured\n");

			__gcov_flush();

			_exit(0);
		}

		// wait for the child to finish
		waitpid(net_child, NULL, 0);
		EUID_USER();
	}
	EUID_ASSERT();

	// close each end of the unused pipes
	close(parent_to_child_fds[0]);
	close(child_to_parent_fds[1]);

	// notify child that base setup is complete
	notify_other(parent_to_child_fds[1]);

	// wait for child to create new user namespace with CLONE_NEWUSER
	wait_for_other(child_to_parent_fds[0]);
	close(child_to_parent_fds[0]);

	if (arg_noroot) {
		// update the UID and GID maps in the new child user namespace
		// uid
		char *map_path;
		if (asprintf(&map_path, "/proc/%d/uid_map", child) == -1)
			errExit("asprintf");

		char *map;
		uid_t uid = getuid();
		if (asprintf(&map, "%d %d 1", uid, uid) == -1)
			errExit("asprintf");
		EUID_ROOT();
		update_map(map, map_path);
		EUID_USER();
		free(map);
		free(map_path);

		// gid file
		if (asprintf(&map_path, "/proc/%d/gid_map", child) == -1)
			errExit("asprintf");
		char gidmap[1024];
		char *ptr = gidmap;
		*ptr = '\0';

		// add user group
		gid_t gid = getgid();
		sprintf(ptr, "%d %d 1\n", gid, gid);
		ptr += strlen(ptr);

		gid_t g;
		if (!arg_nogroups || !check_can_drop_all_groups()) {
			// add audio group
			if (!arg_nosound) {
				g = get_group_id("audio");
				if (g) {
					sprintf(ptr, "%d %d 1\n", g, g);
					ptr += strlen(ptr);
				}
			}

			// add video group
			if (!arg_novideo) {
				g = get_group_id("video");
				if (g) {
					sprintf(ptr, "%d %d 1\n", g, g);
					ptr += strlen(ptr);
				}
			}

			// add render/vglusers group
			if (!arg_no3d) {
				g = get_group_id("render");
				if (g) {
					sprintf(ptr, "%d %d 1\n", g, g);
					ptr += strlen(ptr);
				}
				g = get_group_id("vglusers");
				if (g) {
					sprintf(ptr, "%d %d 1\n", g, g);
					ptr += strlen(ptr);
				}
			}

			// add lp group
			if (!arg_noprinters) {
				g = get_group_id("lp");
				if (g) {
					sprintf(ptr, "%d %d 1\n", g, g);
					ptr += strlen(ptr);
				}
			}

			// add cdrom/optical groups
			if (!arg_nodvd) {
				g = get_group_id("cdrom");
				if (g) {
					sprintf(ptr, "%d %d 1\n", g, g);
					ptr += strlen(ptr);
				}
				g = get_group_id("optical");
				if (g) {
					sprintf(ptr, "%d %d 1\n", g, g);
					ptr += strlen(ptr);
				}
			}

			// add input group
			if (!arg_noinput) {
				g = get_group_id("input");
				if (g) {
					sprintf(ptr, "%d %d 1\n", g, g);
					ptr += strlen(ptr);
				}
			}
		}

		if (!arg_nogroups) {
			// add firejail group
			g = get_group_id("firejail");
			if (g) {
				sprintf(ptr, "%d %d 1\n", g, g);
				ptr += strlen(ptr);
			}

			// add tty group
			g = get_group_id("tty");
			if (g) {
				sprintf(ptr, "%d %d 1\n", g, g);
				ptr += strlen(ptr);
			}

			// add games group
			g = get_group_id("games");
			if (g) {
				sprintf(ptr, "%d %d 1\n", g, g);
			}
		}

		EUID_ROOT();
		update_map(gidmap, map_path);
		EUID_USER();
		free(map_path);
	}
	EUID_ASSERT();

	// notify child that UID/GID mapping is complete
	notify_other(parent_to_child_fds[1]);
	close(parent_to_child_fds[1]);

	EUID_ROOT();
	if (lockfd_network != -1) {
		flock(lockfd_network, LOCK_UN);
		close(lockfd_network);
	}
	EUID_USER();

	// lock netfilter firewall
	if (arg_netlock) {
		char *cmd;
		if (asprintf(&cmd, "firejail --netlock=%d&", getpid()) == -1)
			errExit("asprintf");
		int rv = system(cmd);
		(void) rv;
		free(cmd);
	}

	int status = 0;
	//*****************************
	// following code is signal-safe

	// handle CTRL-C in parent
	install_handler();

	// wait for the child to finish
	waitpid(child, &status, 0);

	// restore default signal actions
	signal(SIGTERM, SIG_DFL);
	signal(SIGINT, SIG_DFL);

	// end of signal-safe code
	//*****************************



	if (WIFEXITED(status)){
		myexit(WEXITSTATUS(status));
	} else if (WIFSIGNALED(status)) {
		// distinguish fatal signals by adding 128
		myexit(128 + WTERMSIG(status));
	} else {
		myexit(1);
	}

	return 1;
}