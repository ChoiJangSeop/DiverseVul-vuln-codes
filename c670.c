void jiffies_to_timeval(const unsigned long jiffies, struct timeval *value)
{
	/*
	 * Convert jiffies to nanoseconds and separate with
	 * one divide.
	 */
	u64 nsec = (u64)jiffies * TICK_NSEC;
	long tv_usec;

	value->tv_sec = div_long_long_rem(nsec, NSEC_PER_SEC, &tv_usec);
	tv_usec /= NSEC_PER_USEC;
	value->tv_usec = tv_usec;
}