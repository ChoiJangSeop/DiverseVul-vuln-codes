start_bfd_child(void)
{
#ifndef _DEBUG_
	pid_t pid;
	int ret;
	char *syslog_ident;

	/* Initialize child process */
	if (log_file_name)
		flush_log_file();

	pid = fork();

	if (pid < 0) {
		log_message(LOG_INFO, "BFD child process: fork error(%m)");
		return -1;
	} else if (pid) {
		bfd_child = pid;
		log_message(LOG_INFO, "Starting BFD child process, pid=%d",
			    pid);

		/* Start respawning thread */
		thread_add_child(master, bfd_respawn_thread, NULL,
				 pid, TIMER_NEVER);
		return 0;
	}
	prctl(PR_SET_PDEATHSIG, SIGTERM);

	prog_type = PROG_TYPE_BFD;

	/* Close the read end of the event notification pipes */
#ifdef _WITH_VRRP_
	close(bfd_vrrp_event_pipe[0]);
#endif
#ifdef _WITH_LVS_
	close(bfd_checker_event_pipe[0]);
#endif

	initialise_debug_options();

	if ((global_data->instance_name
#if HAVE_DECL_CLONE_NEWNET
			   || global_data->network_namespace
#endif
					       ) &&
	     (bfd_syslog_ident = make_syslog_ident(PROG_BFD)))
		syslog_ident = bfd_syslog_ident;
	else
		syslog_ident = PROG_BFD;

	/* Opening local BFD syslog channel */
	if (!__test_bit(NO_SYSLOG_BIT, &debug))
		openlog(syslog_ident, LOG_PID | ((__test_bit(LOG_CONSOLE_BIT, &debug)) ? LOG_CONS : 0)
				    , (log_facility==LOG_DAEMON) ? LOG_LOCAL2 : log_facility);

	if (log_file_name)
		open_log_file(log_file_name,
				"bfd",
#if HAVE_DECL_CLONE_NEWNET
				global_data->network_namespace,
#else
				NULL,
#endif
				global_data->instance_name);

#ifdef _MEM_CHECK_
	mem_log_init(PROG_BFD, "BFD child process");
#endif

	free_parent_mallocs_startup(true);

	/* Clear any child finder functions set in parent */
	set_child_finder_name(NULL);

	/* Child process part, write pidfile */
	if (!pidfile_write(bfd_pidfile, getpid())) {
		/* Fatal error */
		log_message(LOG_INFO,
			    "BFD child process: cannot write pidfile");
		exit(0);
	}

	/* Create the new master thread */
	thread_destroy_master(master);
	master = thread_make_master();

	/* change to / dir */
	ret = chdir("/");
	if (ret < 0) {
		log_message(LOG_INFO, "BFD child process: error chdir");
	}

	/* Set mask */
	umask(0);
#endif

	/* If last process died during a reload, we can get there and we
	 * don't want to loop again, because we're not reloading anymore.
	 */
	UNSET_RELOAD;

#ifndef _DEBUG_
	/* Signal handling initialization */
	bfd_signal_init();
#endif

	/* Start BFD daemon */
	start_bfd(NULL);

#ifdef _DEBUG_
	return 0;
#else

#ifdef THREAD_DUMP
	register_bfd_thread_addresses();
#endif

	/* Launch the scheduling I/O multiplexer */
	launch_thread_scheduler(master);

#ifdef THREAD_DUMP
	deregister_thread_addresses();
#endif

	/* Finish BFD daemon process */
	stop_bfd(EXIT_SUCCESS);

	/* unreachable */
	exit(EXIT_SUCCESS);
#endif
}