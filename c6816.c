xdaemon(bool nochdir, bool noclose, bool exitflag)
{
	pid_t pid;
	int ret;

	if (log_file_name)
		flush_log_file();

	/* In case of fork is error. */
	pid = fork();
	if (pid < 0) {
		log_message(LOG_INFO, "xdaemon: fork error");
		return -1;
	}

	/* In case of this is parent process. */
	if (pid != 0) {
		if (!exitflag)
			exit(0);
		else
			return pid;
	}

	/* Become session leader and get pid. */
	pid = setsid();
	if (pid < -1) {
		log_message(LOG_INFO, "xdaemon: setsid error");
		return -1;
	}

	/* Change directory to root. */
	if (!nochdir) {
		ret = chdir("/");
		if (ret < 0) {
			log_message(LOG_INFO, "xdaemon: chdir error");
		}
	}

	/* File descriptor close. */
	if (!noclose)
		set_std_fd(true);

	return 0;
}