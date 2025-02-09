struct timespec ns_to_timespec(const s64 nsec)
{
	struct timespec ts;

	if (!nsec)
		return (struct timespec) {0, 0};

	ts.tv_sec = div_long_long_rem_signed(nsec, NSEC_PER_SEC, &ts.tv_nsec);
	if (unlikely(nsec < 0))
		set_normalized_timespec(&ts, ts.tv_sec, ts.tv_nsec);

	return ts;
}