static int evtchn_fifo_percpu_deinit(unsigned int cpu)
{
	__evtchn_fifo_handle_events(cpu, true);
	return 0;
}