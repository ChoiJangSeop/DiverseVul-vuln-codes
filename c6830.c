window_pane_spawn(struct window_pane *wp, int argc, char **argv,
    const char *path, const char *shell, const char *cwd, struct environ *env,
    struct termios *tio, char **cause)
{
	struct winsize	 ws;
	char		*argv0, *cmd, **argvp;
	const char	*ptr, *first, *home;
	struct termios	 tio2;
	sigset_t	 set, oldset;

	if (wp->fd != -1) {
		bufferevent_free(wp->event);
		close(wp->fd);
	}
	if (argc > 0) {
		cmd_free_argv(wp->argc, wp->argv);
		wp->argc = argc;
		wp->argv = cmd_copy_argv(argc, argv);
	}
	if (shell != NULL) {
		free(wp->shell);
		wp->shell = xstrdup(shell);
	}
	if (cwd != NULL) {
		free((void *)wp->cwd);
		wp->cwd = xstrdup(cwd);
	}
	wp->flags &= ~(PANE_STATUSREADY|PANE_STATUSDRAWN);

	cmd = cmd_stringify_argv(wp->argc, wp->argv);
	log_debug("%s: shell=%s", __func__, wp->shell);
	log_debug("%s: cmd=%s", __func__, cmd);
	log_debug("%s: cwd=%s", __func__, cwd);
	cmd_log_argv(wp->argc, wp->argv, __func__);
	environ_log(env, "%s: environment ", __func__);

	memset(&ws, 0, sizeof ws);
	ws.ws_col = screen_size_x(&wp->base);
	ws.ws_row = screen_size_y(&wp->base);

	sigfillset(&set);
	sigprocmask(SIG_BLOCK, &set, &oldset);
	switch (wp->pid = fdforkpty(ptm_fd, &wp->fd, wp->tty, NULL, &ws)) {
	case -1:
		wp->fd = -1;

		xasprintf(cause, "%s: %s", cmd, strerror(errno));
		free(cmd);

		sigprocmask(SIG_SETMASK, &oldset, NULL);
		return (-1);
	case 0:
		proc_clear_signals(server_proc, 1);
		sigprocmask(SIG_SETMASK, &oldset, NULL);

		cwd = NULL;
		if (chdir(wp->cwd) == 0)
			cwd = wp->cwd;
		else if ((home = find_home()) != NULL && chdir(home) == 0)
			cwd = home;
		else
			chdir("/");

		if (tcgetattr(STDIN_FILENO, &tio2) != 0)
			fatal("tcgetattr failed");
		if (tio != NULL)
			memcpy(tio2.c_cc, tio->c_cc, sizeof tio2.c_cc);
		tio2.c_cc[VERASE] = '\177';
		if (tcsetattr(STDIN_FILENO, TCSANOW, &tio2) != 0)
			fatal("tcgetattr failed");

		log_close();
		closefrom(STDERR_FILENO + 1);

		if (path != NULL)
			environ_set(env, "PATH", "%s", path);
		if (cwd != NULL)
			environ_set(env, "PWD", "%s", cwd);
		environ_set(env, "TMUX_PANE", "%%%u", wp->id);
		environ_push(env);

		setenv("SHELL", wp->shell, 1);
		ptr = strrchr(wp->shell, '/');

		/*
		 * If given one argument, assume it should be passed to sh -c;
		 * with more than one argument, use execvp(). If there is no
		 * arguments, create a login shell.
		 */
		if (wp->argc > 0) {
			if (wp->argc != 1) {
				/* Copy to ensure argv ends in NULL. */
				argvp = cmd_copy_argv(wp->argc, wp->argv);
				execvp(argvp[0], argvp);
				fatal("execvp failed");
			}
			first = wp->argv[0];

			if (ptr != NULL && *(ptr + 1) != '\0')
				xasprintf(&argv0, "%s", ptr + 1);
			else
				xasprintf(&argv0, "%s", wp->shell);
			execl(wp->shell, argv0, "-c", first, (char *)NULL);
			fatal("execl failed");
		}
		if (ptr != NULL && *(ptr + 1) != '\0')
			xasprintf(&argv0, "-%s", ptr + 1);
		else
			xasprintf(&argv0, "-%s", wp->shell);
		execl(wp->shell, argv0, (char *)NULL);
		fatal("execl failed");
	}
	log_debug("%s: master=%s", __func__, ttyname(wp->fd));
	log_debug("%s: slave=%s", __func__, wp->tty);

	sigprocmask(SIG_SETMASK, &oldset, NULL);
	setblocking(wp->fd, 0);

	wp->event = bufferevent_new(wp->fd, window_pane_read_callback, NULL,
	    window_pane_error_callback, wp);

	bufferevent_setwatermark(wp->event, EV_READ, 0, READ_SIZE);
	bufferevent_enable(wp->event, EV_READ|EV_WRITE);

	free(cmd);
	return (0);
}