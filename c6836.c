cmd_pipe_pane_exec(struct cmd *self, struct cmdq_item *item)
{
	struct args		*args = self->args;
	struct client		*c = cmd_find_client(item, NULL, 1);
	struct window_pane	*wp = item->target.wp;
	struct session		*s = item->target.s;
	struct winlink		*wl = item->target.wl;
	char			*cmd;
	int			 old_fd, pipe_fd[2], null_fd, in, out;
	struct format_tree	*ft;
	sigset_t		 set, oldset;

	/* Destroy the old pipe. */
	old_fd = wp->pipe_fd;
	if (wp->pipe_fd != -1) {
		bufferevent_free(wp->pipe_event);
		close(wp->pipe_fd);
		wp->pipe_fd = -1;

		if (window_pane_destroy_ready(wp)) {
			server_destroy_pane(wp, 1);
			return (CMD_RETURN_NORMAL);
		}
	}

	/* If no pipe command, that is enough. */
	if (args->argc == 0 || *args->argv[0] == '\0')
		return (CMD_RETURN_NORMAL);

	/*
	 * With -o, only open the new pipe if there was no previous one. This
	 * allows a pipe to be toggled with a single key, for example:
	 *
	 *	bind ^p pipep -o 'cat >>~/output'
	 */
	if (args_has(self->args, 'o') && old_fd != -1)
		return (CMD_RETURN_NORMAL);

	/* What do we want to do? Neither -I or -O is -O. */
	if (args_has(self->args, 'I')) {
		in = 1;
		out = args_has(self->args, 'O');
	} else {
		in = 0;
		out = 1;
	}

	/* Open the new pipe. */
	if (socketpair(AF_UNIX, SOCK_STREAM, PF_UNSPEC, pipe_fd) != 0) {
		cmdq_error(item, "socketpair error: %s", strerror(errno));
		return (CMD_RETURN_ERROR);
	}

	/* Expand the command. */
	ft = format_create(item->client, item, FORMAT_NONE, 0);
	format_defaults(ft, c, s, wl, wp);
	cmd = format_expand_time(ft, args->argv[0], time(NULL));
	format_free(ft);

	/* Fork the child. */
	sigfillset(&set);
	sigprocmask(SIG_BLOCK, &set, &oldset);
	switch (fork()) {
	case -1:
		sigprocmask(SIG_SETMASK, &oldset, NULL);
		cmdq_error(item, "fork error: %s", strerror(errno));

		free(cmd);
		return (CMD_RETURN_ERROR);
	case 0:
		/* Child process. */
		proc_clear_signals(server_proc, 1);
		sigprocmask(SIG_SETMASK, &oldset, NULL);
		close(pipe_fd[0]);

		null_fd = open(_PATH_DEVNULL, O_WRONLY, 0);
		if (out) {
			if (dup2(pipe_fd[1], STDIN_FILENO) == -1)
				_exit(1);
		} else {
			if (dup2(null_fd, STDIN_FILENO) == -1)
				_exit(1);
		}
		if (in) {
			if (dup2(pipe_fd[1], STDOUT_FILENO) == -1)
				_exit(1);
			if (pipe_fd[1] != STDOUT_FILENO)
				close(pipe_fd[1]);
		} else {
			if (dup2(null_fd, STDOUT_FILENO) == -1)
				_exit(1);
		}
		if (dup2(null_fd, STDERR_FILENO) == -1)
			_exit(1);
		closefrom(STDERR_FILENO + 1);

		execl(_PATH_BSHELL, "sh", "-c", cmd, (char *) NULL);
		_exit(1);
	default:
		/* Parent process. */
		sigprocmask(SIG_SETMASK, &oldset, NULL);
		close(pipe_fd[1]);

		wp->pipe_fd = pipe_fd[0];
		wp->pipe_off = EVBUFFER_LENGTH(wp->event->input);

		setblocking(wp->pipe_fd, 0);
		wp->pipe_event = bufferevent_new(wp->pipe_fd,
		    cmd_pipe_pane_read_callback,
		    cmd_pipe_pane_write_callback,
		    cmd_pipe_pane_error_callback,
		    wp);
		if (out)
			bufferevent_enable(wp->pipe_event, EV_WRITE);
		if (in)
			bufferevent_enable(wp->pipe_event, EV_READ);

		free(cmd);
		return (CMD_RETURN_NORMAL);
	}
}