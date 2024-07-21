vrrp_terminate_phase2(int exit_status)
{
	struct rusage usage;

#ifdef _NETLINK_TIMERS_
	if (do_netlink_timers)
		report_and_clear_netlink_timers("Starting shutdown instances");
#endif

	if (!__test_bit(DONT_RELEASE_VRRP_BIT, &debug))
		shutdown_vrrp_instances();

#ifdef _NETLINK_TIMERS_
	if (do_netlink_timers)
		report_and_clear_netlink_timers("Completed shutdown instances");
#endif

#if defined _WITH_SNMP_RFC || defined _WITH_SNMP_VRRP_
	if (
#ifdef _WITH_SNMP_RFC_
	    global_data->enable_snmp_vrrp ||
#endif
#ifdef _WITH_SNMP_RFCV2_
	    global_data->enable_snmp_rfcv2 ||
#endif
#ifdef _WITH_SNMP_RFCV3_
	    global_data->enable_snmp_rfcv3 ||
#endif
	    false)
		vrrp_snmp_agent_close();
#endif

#ifdef _WITH_LVS_
	if (vrrp_ipvs_needed()) {
		/* Clean ipvs related */
		ipvs_stop();
	}
#endif

	kernel_netlink_close_cmd();
	thread_destroy_master(master);
	master = NULL;
	gratuitous_arp_close();
	ndisc_close();

#ifdef _WITH_DBUS_
	if (global_data->enable_dbus)
		dbus_stop();
#endif

	if (global_data->vrrp_notify_fifo.fd != -1)
		notify_fifo_close(&global_data->notify_fifo, &global_data->vrrp_notify_fifo);

	free_global_data(global_data);
	free_vrrp_data(vrrp_data);
	free_vrrp_buffer();
	free_interface_queue();
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
	FREE_PTR(vrrp_syslog_ident);
#else
	if (vrrp_syslog_ident)
		free(vrrp_syslog_ident);
#endif
	close_std_fd();

	/* Stop daemon */
	pidfile_rm(vrrp_pidfile);

	exit(exit_status);
}