static int start_client(int argc, char *argv[])
{
	char *p, *shell_machine = NULL, *shell_user = NULL;
	char **remote_argv;
	int remote_argc, env_port = rsync_port;
	int f_in, f_out;
	int ret;
	pid_t pid;

	/* Don't clobber argv[] so that ps(1) can still show the right
	 * command line. */
	dup_argv(argv);

	if (!read_batch) { /* for read_batch, NO source is specified */
		char *path = check_for_hostspec(argv[0], &shell_machine, &rsync_port);
		if (path) { /* source is remote */
			char *dummy_host;
			int dummy_port = 0;
			*argv = path;
			remote_argv = argv;
			remote_argc = argc;
			argv += argc - 1;
			if (argc == 1 || **argv == ':')
				argc = 0; /* no dest arg */
			else if (check_for_hostspec(*argv, &dummy_host, &dummy_port)) {
				rprintf(FERROR,
					"The source and destination cannot both be remote.\n");
				exit_cleanup(RERR_SYNTAX);
			} else {
				remote_argc--; /* don't count dest */
				argc = 1;
			}
			if (filesfrom_host && *filesfrom_host && strcmp(filesfrom_host, shell_machine) != 0) {
				rprintf(FERROR,
					"--files-from hostname is not the same as the transfer hostname\n");
				exit_cleanup(RERR_SYNTAX);
			}
			am_sender = 0;
			if (rsync_port)
				daemon_connection = shell_cmd ? 1 : -1;
		} else { /* source is local, check dest arg */
			am_sender = 1;

			if (argc > 1) {
				p = argv[--argc];
				remote_argv = argv + argc;
			} else {
				static char *dotarg[1] = { "." };
				p = dotarg[0];
				remote_argv = dotarg;
			}
			remote_argc = 1;

			path = check_for_hostspec(p, &shell_machine, &rsync_port);
			if (path && filesfrom_host && *filesfrom_host && strcmp(filesfrom_host, shell_machine) != 0) {
				rprintf(FERROR,
					"--files-from hostname is not the same as the transfer hostname\n");
				exit_cleanup(RERR_SYNTAX);
			}
			if (!path) { /* no hostspec found, so src & dest are local */
				local_server = 1;
				if (filesfrom_host) {
					rprintf(FERROR,
						"--files-from cannot be remote when the transfer is local\n");
					exit_cleanup(RERR_SYNTAX);
				}
				shell_machine = NULL;
				rsync_port = 0;
			} else { /* hostspec was found, so dest is remote */
				argv[argc] = path;
				if (rsync_port)
					daemon_connection = shell_cmd ? 1 : -1;
			}
		}
	} else {  /* read_batch */
		local_server = 1;
		if (check_for_hostspec(argv[argc-1], &shell_machine, &rsync_port)) {
			rprintf(FERROR, "remote destination is not allowed with --read-batch\n");
			exit_cleanup(RERR_SYNTAX);
		}
		remote_argv = argv += argc - 1;
		remote_argc = argc = 1;
		rsync_port = 0;
	}

	/* A local transfer doesn't unbackslash anything, so leave the args alone. */
	if (local_server)
		old_style_args = 2;

	if (!rsync_port && remote_argc && !**remote_argv) /* Turn an empty arg into a dot dir. */
		*remote_argv = ".";

	if (am_sender) {
		char *dummy_host;
		int dummy_port = rsync_port;
		int i;
		if (!argv[0][0])
			goto invalid_empty;
		/* For local source, extra source args must not have hostspec. */
		for (i = 1; i < argc; i++) {
			if (!argv[i][0]) {
			    invalid_empty:
				rprintf(FERROR, "Empty source arg specified.\n");
				exit_cleanup(RERR_SYNTAX);
			}
			if (check_for_hostspec(argv[i], &dummy_host, &dummy_port)) {
				rprintf(FERROR, "Unexpected remote arg: %s\n", argv[i]);
				exit_cleanup(RERR_SYNTAX);
			}
		}
	} else {
		char *dummy_host;
		int dummy_port = rsync_port;
		int i;
		/* For remote source, any extra source args must have either
		 * the same hostname or an empty hostname. */
		for (i = 1; i < remote_argc; i++) {
			char *arg = check_for_hostspec(remote_argv[i], &dummy_host, &dummy_port);
			if (!arg) {
				rprintf(FERROR, "Unexpected local arg: %s\n", remote_argv[i]);
				rprintf(FERROR, "If arg is a remote file/dir, prefix it with a colon (:).\n");
				exit_cleanup(RERR_SYNTAX);
			}
			if (*dummy_host && strcmp(dummy_host, shell_machine) != 0) {
				rprintf(FERROR, "All source args must come from the same machine.\n");
				exit_cleanup(RERR_SYNTAX);
			}
			if (rsync_port != dummy_port) {
				if (!rsync_port || !dummy_port)
					rprintf(FERROR, "All source args must use the same hostspec format.\n");
				else
					rprintf(FERROR, "All source args must use the same port number.\n");
				exit_cleanup(RERR_SYNTAX);
			}
			if (!rsync_port && !*arg) /* Turn an empty arg into a dot dir. */
				arg = ".";
			remote_argv[i] = arg;
		}
	}

	if (rsync_port < 0)
		rsync_port = RSYNC_PORT;
	else
		env_port = rsync_port;

	if (daemon_connection < 0)
		return start_socket_client(shell_machine, remote_argc, remote_argv, argc, argv);

	if (password_file && !daemon_connection) {
		rprintf(FERROR, "The --password-file option may only be "
				"used when accessing an rsync daemon.\n");
		exit_cleanup(RERR_SYNTAX);
	}

	if (connect_timeout) {
		rprintf(FERROR, "The --contimeout option may only be "
				"used when connecting to an rsync daemon.\n");
		exit_cleanup(RERR_SYNTAX);
	}

	if (shell_machine) {
		p = strrchr(shell_machine,'@');
		if (p) {
			*p = 0;
			shell_user = shell_machine;
			shell_machine = p+1;
		}
	}

	if (DEBUG_GTE(CMD, 2)) {
		rprintf(FINFO,"cmd=%s machine=%s user=%s path=%s\n",
			NS(shell_cmd), NS(shell_machine), NS(shell_user),
			NS(remote_argv[0]));
	}

#ifdef HAVE_PUTENV
	if (daemon_connection)
		set_env_num("RSYNC_PORT", env_port);
#else
	(void)env_port;
#endif

	pid = do_cmd(shell_cmd, shell_machine, shell_user, remote_argv, remote_argc, &f_in, &f_out);

	/* if we're running an rsync server on the remote host over a
	 * remote shell command, we need to do the RSYNCD protocol first */
	if (daemon_connection) {
		int tmpret;
		tmpret = start_inband_exchange(f_in, f_out, shell_user, remote_argc, remote_argv);
		if (tmpret < 0)
			return tmpret;
	}

	ret = client_run(f_in, f_out, pid, argc, argv);

	fflush(stdout);
	fflush(stderr);

	return ret;
}