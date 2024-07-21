modprobe_ipvs(void)
{
	char *argv[] = { "/sbin/modprobe", "-s", "--", "ip_vs", NULL };
	int child;
	int status;
	int rc;
	char *modprobe = get_modprobe();
	struct sigaction act, old_act;

	if (modprobe)
		argv[0] = modprobe;

	act.sa_handler = SIG_DFL;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;

	sigaction ( SIGCHLD, &act, &old_act);

	if (log_file_name)
		flush_log_file();

	if (!(child = fork())) {
		execv(argv[0], argv);
		exit(1);
	}

	rc = waitpid(child, &status, 0);

	sigaction ( SIGCHLD, &old_act, NULL);

	if (rc < 0) {
		log_message(LOG_INFO, "IPVS: waitpid error (%s)"
				    , strerror(errno));
	}

	if (modprobe)
		FREE(modprobe);

	if (!WIFEXITED(status) || WEXITSTATUS(status)) {
		return true;
	}

	return false;
}