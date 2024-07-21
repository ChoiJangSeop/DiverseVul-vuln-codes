job_run(const char *cmd, struct session *s, const char *cwd,
    job_update_cb updatecb, job_complete_cb completecb, job_free_cb freecb,
    void *data, int flags)
{
	struct job	*job;
	struct environ	*env;
	pid_t		 pid;
	int		 nullfd, out[2];
	const char	*home;
	sigset_t	 set, oldset;

	if (socketpair(AF_UNIX, SOCK_STREAM, PF_UNSPEC, out) != 0)
		return (NULL);
	log_debug("%s: cmd=%s, cwd=%s", __func__, cmd, cwd == NULL ? "" : cwd);

	/*
	 * Do not set TERM during .tmux.conf, it is nice to be able to use
	 * if-shell to decide on default-terminal based on outside TERM.
	 */
	env = environ_for_session(s, !cfg_finished);

	sigfillset(&set);
	sigprocmask(SIG_BLOCK, &set, &oldset);
	switch (pid = fork()) {
	case -1:
		sigprocmask(SIG_SETMASK, &oldset, NULL);
		environ_free(env);
		close(out[0]);
		close(out[1]);
		return (NULL);
	case 0:
		proc_clear_signals(server_proc, 1);
		sigprocmask(SIG_SETMASK, &oldset, NULL);

		if (cwd == NULL || chdir(cwd) != 0) {
			if ((home = find_home()) == NULL || chdir(home) != 0)
				chdir("/");
		}

		environ_push(env);
		environ_free(env);

		if (dup2(out[1], STDIN_FILENO) == -1)
			fatal("dup2 failed");
		if (dup2(out[1], STDOUT_FILENO) == -1)
			fatal("dup2 failed");
		if (out[1] != STDIN_FILENO && out[1] != STDOUT_FILENO)
			close(out[1]);
		close(out[0]);

		nullfd = open(_PATH_DEVNULL, O_RDWR, 0);
		if (nullfd < 0)
			fatal("open failed");
		if (dup2(nullfd, STDERR_FILENO) == -1)
			fatal("dup2 failed");
		if (nullfd != STDERR_FILENO)
			close(nullfd);

		closefrom(STDERR_FILENO + 1);

		execl(_PATH_BSHELL, "sh", "-c", cmd, (char *) NULL);
		fatal("execl failed");
	}

	sigprocmask(SIG_SETMASK, &oldset, NULL);
	environ_free(env);
	close(out[1]);

	job = xmalloc(sizeof *job);
	job->state = JOB_RUNNING;
	job->flags = flags;

	job->cmd = xstrdup(cmd);
	job->pid = pid;
	job->status = 0;

	LIST_INSERT_HEAD(&all_jobs, job, entry);

	job->updatecb = updatecb;
	job->completecb = completecb;
	job->freecb = freecb;
	job->data = data;

	job->fd = out[0];
	setblocking(job->fd, 0);

	job->event = bufferevent_new(job->fd, job_read_callback,
	    job_write_callback, job_error_callback, job);
	bufferevent_enable(job->event, EV_READ|EV_WRITE);

	log_debug("run job %p: %s, pid %ld", job, job->cmd, (long) job->pid);
	return (job);
}