u64 sched_clock_cpu(int cpu)
{
	if (unlikely(!sched_clock_running))
		return 0;

	return sched_clock();
}