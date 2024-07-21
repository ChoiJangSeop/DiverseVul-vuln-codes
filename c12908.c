static pid_t do_cmd(char *cmd, char *machine, char *user, char **remote_argv, int remote_argc,
		    int *f_in_p, int *f_out_p)
{
	int i, argc = 0;
	char *args[MAX_ARGS], *need_to_free = NULL;
	pid_t pid;
	int dash_l_set = 0;

	if (!read_batch && !local_server) {
		char *t, *f, in_quote = '\0';
		char *rsh_env = getenv(RSYNC_RSH_ENV);
		if (!cmd)
			cmd = rsh_env;
		if (!cmd)
			cmd = RSYNC_RSH;
		cmd = need_to_free = strdup(cmd);

		for (t = f = cmd; *f; f++) {
			if (*f == ' ')
				continue;
			/* Comparison leaves rooms for server_options(). */
			if (argc >= MAX_ARGS - MAX_SERVER_ARGS)
				goto arg_overflow;
			args[argc++] = t;
			while (*f != ' ' || in_quote) {
				if (!*f) {
					if (in_quote) {
						rprintf(FERROR,
							"Missing trailing-%c in remote-shell command.\n",
							in_quote);
						exit_cleanup(RERR_SYNTAX);
					}
					f--;
					break;
				}
				if (*f == '\'' || *f == '"') {
					if (!in_quote) {
						in_quote = *f++;
						continue;
					}
					if (*f == in_quote && *++f != in_quote) {
						in_quote = '\0';
						continue;
					}
				}
				*t++ = *f++;
			}
			*t++ = '\0';
		}

		/* NOTE: must preserve t == start of command name until the end of the args handling! */
		if ((t = strrchr(cmd, '/')) != NULL)
			t++;
		else
			t = cmd;

		/* Check to see if we've already been given '-l user' in the remote-shell command. */
		for (i = 0; i < argc-1; i++) {
			if (!strcmp(args[i], "-l") && args[i+1][0] != '-')
				dash_l_set = 1;
		}

#ifdef HAVE_REMSH
		/* remsh (on HPUX) takes the arguments the other way around */
		args[argc++] = machine;
		if (user && !(daemon_connection && dash_l_set)) {
			args[argc++] = "-l";
			args[argc++] = user;
		}
#else
		if (user && !(daemon_connection && dash_l_set)) {
			args[argc++] = "-l";
			args[argc++] = user;
		}
#ifdef AF_INET
		if (default_af_hint == AF_INET && strcmp(t, "ssh") == 0)
			args[argc++] = "-4"; /* we're using ssh so we can add a -4 option */
#endif
#ifdef AF_INET6
		if (default_af_hint == AF_INET6 && strcmp(t, "ssh") == 0)
			args[argc++] = "-6"; /* we're using ssh so we can add a -6 option */
#endif
		args[argc++] = machine;
#endif

		args[argc++] = rsync_path;

		if (blocking_io < 0 && (strcmp(t, "rsh") == 0 || strcmp(t, "remsh") == 0))
			blocking_io = 1;

		if (daemon_connection > 0) {
			args[argc++] = "--server";
			args[argc++] = "--daemon";
		} else
			server_options(args, &argc);

		if (argc >= MAX_ARGS - 2)
			goto arg_overflow;
	}

	args[argc++] = ".";

	if (!daemon_connection) {
		while (remote_argc > 0) {
			if (argc >= MAX_ARGS - 1) {
			  arg_overflow:
				rprintf(FERROR, "internal: args[] overflowed in do_cmd()\n");
				exit_cleanup(RERR_SYNTAX);
			}
			args[argc++] = safe_arg(NULL, *remote_argv++);
			remote_argc--;
		}
	}

	args[argc] = NULL;

	if (DEBUG_GTE(CMD, 2)) {
		for (i = 0; i < argc; i++)
			rprintf(FCLIENT, "cmd[%d]=%s ", i, args[i]);
		rprintf(FCLIENT, "\n");
	}

	if (read_batch) {
		int from_gen_pipe[2];
		set_allow_inc_recurse();
		if (fd_pair(from_gen_pipe) < 0) {
			rsyserr(FERROR, errno, "pipe");
			exit_cleanup(RERR_IPC);
		}
		batch_gen_fd = from_gen_pipe[0];
		*f_out_p = from_gen_pipe[1];
		*f_in_p = batch_fd;
		pid = (pid_t)-1; /* no child pid */
#ifdef ICONV_CONST
		setup_iconv();
#endif
	} else if (local_server) {
		/* If the user didn't request --[no-]whole-file, force
		 * it on, but only if we're not batch processing. */
		if (whole_file < 0 && !write_batch)
			whole_file = 1;
		set_allow_inc_recurse();
		pid = local_child(argc, args, f_in_p, f_out_p, child_main);
#ifdef ICONV_CONST
		setup_iconv();
#endif
	} else {
		pid = piped_child(args, f_in_p, f_out_p);
#ifdef ICONV_CONST
		setup_iconv();
#endif
		if (protect_args && !daemon_connection)
			send_protected_args(*f_out_p, args);
	}

	if (need_to_free)
		free(need_to_free);

	return pid;
}