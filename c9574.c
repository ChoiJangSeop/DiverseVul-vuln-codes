static void __evtchn_fifo_handle_events(unsigned cpu, bool drop)
{
	struct evtchn_fifo_control_block *control_block;
	unsigned long ready;
	unsigned q;

	control_block = per_cpu(cpu_control_block, cpu);

	ready = xchg(&control_block->ready, 0);

	while (ready) {
		q = find_first_bit(&ready, EVTCHN_FIFO_MAX_QUEUES);
		consume_one_event(cpu, control_block, q, &ready, drop);
		ready |= xchg(&control_block->ready, 0);
	}
}