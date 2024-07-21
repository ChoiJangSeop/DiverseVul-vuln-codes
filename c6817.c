fork_exec(char **argv)
{
	pid_t pid;
	int status;
	struct sigaction act, old_act;
	int res = 0;

	act.sa_handler = SIG_DFL;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;

	sigaction(SIGCHLD, &act, &old_act);

	if (log_file_name)
		flush_log_file();

	pid = local_fork();
	if (pid < 0)
		res = -1;
	else if (pid == 0) {
		/* Child */
		set_std_fd(false);

		signal_handler_script();

		execvp(*argv, argv);
		exit(EXIT_FAILURE);
	} else {
		/* Parent */
		while (waitpid(pid, &status, 0) != pid);

		if (!WIFEXITED(status) || WEXITSTATUS(status) != EXIT_SUCCESS)
			res = -1;
	}

	sigaction(SIGCHLD, &old_act, NULL);

	return res;
}