start_vrrp_child(void)
{
#ifndef _DEBUG_
	pid_t pid;
	char *syslog_ident;

	/* Initialize child process */
	if (log_file_name)
		flush_log_file();

	pid = fork();

	if (pid < 0) {
		log_message(LOG_INFO, "VRRP child process: fork error(%s)"
			       , strerror(errno));
		return -1;
	} else if (pid) {
		vrrp_child = pid;
		log_message(LOG_INFO, "Starting VRRP child process, pid=%d"
			       , pid);

		/* Start respawning thread */
		thread_add_child(master, vrrp_respawn_thread, NULL,
				 pid, TIMER_NEVER);

		return 0;
	}

	prctl(PR_SET_PDEATHSIG, SIGTERM);

#ifdef _WITH_PERF_
	if (perf_run == PERF_ALL)
		run_perf("vrrp", global_data->network_namespace, global_data->instance_name);
#endif

	prog_type = PROG_TYPE_VRRP;

	initialise_debug_options();

#ifdef _WITH_BFD_
	/* Close the write end of the BFD vrrp event notification pipe */
	close(bfd_vrrp_event_pipe[1]);

#ifdef _WITH_LVS_
	close(bfd_checker_event_pipe[0]);
	close(bfd_checker_event_pipe[1]);
#endif
#endif

	/* Opening local VRRP syslog channel */
	if ((global_data->instance_name
#if HAVE_DECL_CLONE_NEWNET
			   || global_data->network_namespace
#endif
					       ) &&
	    (vrrp_syslog_ident = make_syslog_ident(PROG_VRRP)))
			syslog_ident = vrrp_syslog_ident;
	else
		syslog_ident = PROG_VRRP;

	if (!__test_bit(NO_SYSLOG_BIT, &debug))
		openlog(syslog_ident, LOG_PID | ((__test_bit(LOG_CONSOLE_BIT, &debug)) ? LOG_CONS : 0)
				    , (log_facility==LOG_DAEMON) ? LOG_LOCAL1 : log_facility);

	if (log_file_name)
		open_log_file(log_file_name,
				"vrrp",
#if HAVE_DECL_CLONE_NEWNET
				global_data->network_namespace,
#else
				NULL,
#endif
				global_data->instance_name);

#ifdef _MEM_CHECK_
	mem_log_init(PROG_VRRP, "VRRP Child process");
#endif

	free_parent_mallocs_startup(true);

	/* Clear any child finder functions set in parent */
	set_child_finder_name(NULL);

	/* Child process part, write pidfile */
	if (!pidfile_write(vrrp_pidfile, getpid())) {
		/* Fatal error */
		log_message(LOG_INFO, "VRRP child process: cannot write pidfile");
		exit(0);
	}

#ifdef _VRRP_FD_DEBUG_
	if (do_vrrp_fd_debug)
		set_extra_threads_debug(dump_vrrp_fd);
#endif

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
	vrrp_signal_init();
#endif

	/* Start VRRP daemon */
	start_vrrp(NULL);

#ifdef _DEBUG_
	return 0;
#endif

#ifdef THREAD_DUMP
	register_vrrp_thread_addresses();
#endif

#ifdef _WITH_PERF_
	if (perf_run == PERF_RUN)
		run_perf("vrrp", global_data->network_namespace, global_data->instance_name);
#endif
	/* Launch the scheduling I/O multiplexer */
	launch_thread_scheduler(master);

#ifdef THREAD_DUMP
	deregister_thread_addresses();
#endif

	/* Finish VRRP daemon process */
	vrrp_terminate_phase2(EXIT_SUCCESS);

	/* unreachable */
	exit(EXIT_SUCCESS);
}