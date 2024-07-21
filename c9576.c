static void evtchn_fifo_handle_events(unsigned cpu)
{
	__evtchn_fifo_handle_events(cpu, false);
}