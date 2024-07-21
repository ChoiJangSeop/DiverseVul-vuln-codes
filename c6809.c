start_check_child(void)
{
#ifndef _DEBUG_
	pid_t pid;
	char *syslog_ident;

	/* Initialize child process */
	if (log_file_name)
		flush_log_file();

	pid = fork();

	if (pid < 0) {
		log_message(LOG_INFO, "Healthcheck child process: fork error(%s)"
			       , strerror(errno));
		return -1;
	} else if (pid) {
		checkers_child = pid;
		log_message(LOG_INFO, "Starting Healthcheck child process, pid=%d"
			       , pid);

		/* Start respawning thread */
		thread_add_child(master, check_respawn_thread, NULL,
				 pid, TIMER_NEVER);

		return 0;
	}
	prctl(PR_SET_PDEATHSIG, SIGTERM);

	prog_type = PROG_TYPE_CHECKER;

	initialise_debug_options();

#ifdef _WITH_BFD_
	/* Close the write end of the BFD checker event notification pipe */
	close(bfd_checker_event_pipe[1]);

#ifdef _WITH_VRRP_
	close(bfd_vrrp_event_pipe[0]);
	close(bfd_vrrp_event_pipe[1]);
#endif
#endif

	if ((global_data->instance_name
#if HAVE_DECL_CLONE_NEWNET
			   || global_data->network_namespace
#endif
					       ) &&
	     (check_syslog_ident = make_syslog_ident(PROG_CHECK)))
		syslog_ident = check_syslog_ident;
	else
		syslog_ident = PROG_CHECK;

	/* Opening local CHECK syslog channel */
	if (!__test_bit(NO_SYSLOG_BIT, &debug))
		openlog(syslog_ident, LOG_PID | ((__test_bit(LOG_CONSOLE_BIT, &debug)) ? LOG_CONS : 0)
				    , (log_facility==LOG_DAEMON) ? LOG_LOCAL2 : log_facility);

	if (log_file_name)
		open_log_file(log_file_name,
				"check",
#if HAVE_DECL_CLONE_NEWNET
				global_data->network_namespace,
#else
				NULL,
#endif
				global_data->instance_name);

#ifdef _MEM_CHECK_
	mem_log_init(PROG_CHECK, "Healthcheck child process");
#endif

	free_parent_mallocs_startup(true);

	/* Clear any child finder functions set in parent */
	set_child_finder_name(NULL);

	/* Child process part, write pidfile */
	if (!pidfile_write(checkers_pidfile, getpid())) {
		log_message(LOG_INFO, "Healthcheck child process: cannot write pidfile");
		exit(KEEPALIVED_EXIT_FATAL);
	}

	/* Create the new master thread */
	thread_destroy_master(master);	/* This destroys any residual settings from the parent */
	master = thread_make_master();
#endif

	/* If last process died during a reload, we can get there and we
	 * don't want to loop again, because we're not reloading anymore.
	 */
	UNSET_RELOAD;

#ifndef _DEBUG_
	/* Signal handling initialization */
	check_signal_init();
#endif

	/* Start Healthcheck daemon */
	start_check(NULL, NULL);

#ifdef _DEBUG_
	return 0;
#endif

#ifdef THREAD_DUMP
	register_check_thread_addresses();
#endif

	/* Launch the scheduling I/O multiplexer */
	launch_thread_scheduler(master);

	/* Finish healthchecker daemon process */
	if (two_phase_terminate)
		checker_terminate_phase2();
	else
		stop_check(KEEPALIVED_EXIT_OK);

#ifdef THREAD_DUMP
	deregister_thread_addresses();
#endif

	/* unreachable */
	exit(KEEPALIVED_EXIT_OK);
}