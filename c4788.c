void __init init_timer_stats(void)
{
	int cpu;

	for_each_possible_cpu(cpu)
		raw_spin_lock_init(&per_cpu(tstats_lookup_lock, cpu));
}