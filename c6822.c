stop_bfd(int status)
{
	struct rusage usage;

	if (__test_bit(CONFIG_TEST_BIT, &debug))
		return;

	/* Stop daemon */
	pidfile_rm(bfd_pidfile);

	/* Clean data */
	free_global_data(global_data);
	bfd_dispatcher_release(bfd_data);
	free_bfd_data(bfd_data);
	free_bfd_buffer();
	thread_destroy_master(master);
	free_parent_mallocs_exit();

	/*
	 * Reached when terminate signal catched.
	 * finally return to parent process.
	 */
	if (__test_bit(LOG_DETAIL_BIT, &debug)) {
		getrusage(RUSAGE_SELF, &usage);
		log_message(LOG_INFO, "Stopped - used %ld.%6.6ld user time, %ld.%6.6ld system time", usage.ru_utime.tv_sec, usage.ru_utime.tv_usec, usage.ru_stime.tv_sec, usage.ru_stime.tv_usec);
	}
	else
		log_message(LOG_INFO, "Stopped");

	if (log_file_name)
		close_log_file();
	closelog();

#ifndef _MEM_CHECK_LOG_
	FREE_PTR(bfd_syslog_ident);
#else
	if (bfd_syslog_ident)
		free(bfd_syslog_ident);
#endif
	close_std_fd();

	exit(status);
}