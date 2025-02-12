static int sgi_clock_get(clockid_t clockid, struct timespec *tp)
{
	u64 nsec;

	nsec = rtc_time() * sgi_clock_period
			+ sgi_clock_offset.tv_nsec;
	tp->tv_sec = div_long_long_rem(nsec, NSEC_PER_SEC, &tp->tv_nsec)
			+ sgi_clock_offset.tv_sec;
	return 0;
};