notify_exec(const notify_script_t *script)
{
	pid_t pid;

	if (log_file_name)
		flush_log_file();

	pid = local_fork();

	if (pid < 0) {
		/* fork error */
		log_message(LOG_INFO, "Failed fork process");
		return -1;
	}

	if (pid) {
		/* parent process */
		return 0;
	}

#ifdef _MEM_CHECK_
	skip_mem_dump();
#endif

	system_call(script);

	/* We should never get here */
	exit(0);
}