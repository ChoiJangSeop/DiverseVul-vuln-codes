static void sample_to_timespec(const clockid_t which_clock,
			       union cpu_time_count cpu,
			       struct timespec *tp)
{
	if (CPUCLOCK_WHICH(which_clock) == CPUCLOCK_SCHED) {
		tp->tv_sec = div_long_long_rem(cpu.sched,
					       NSEC_PER_SEC, &tp->tv_nsec);
	} else {
		cputime_to_timespec(cpu.cpu, tp);
	}
}