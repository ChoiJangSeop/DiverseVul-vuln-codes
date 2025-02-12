static int alarm_timer_nsleep(const clockid_t which_clock, int flags,
			      const struct timespec64 *tsreq)
{
	enum  alarmtimer_type type = clock2alarm(which_clock);
	struct restart_block *restart = &current->restart_block;
	struct alarm alarm;
	ktime_t exp;
	int ret = 0;

	if (!alarmtimer_get_rtcdev())
		return -ENOTSUPP;

	if (flags & ~TIMER_ABSTIME)
		return -EINVAL;

	if (!capable(CAP_WAKE_ALARM))
		return -EPERM;

	alarm_init_on_stack(&alarm, type, alarmtimer_nsleep_wakeup);

	exp = timespec64_to_ktime(*tsreq);
	/* Convert (if necessary) to absolute time */
	if (flags != TIMER_ABSTIME) {
		ktime_t now = alarm_bases[type].gettime();
		exp = ktime_add(now, exp);
	}

	ret = alarmtimer_do_nsleep(&alarm, exp, type);
	if (ret != -ERESTART_RESTARTBLOCK)
		return ret;

	/* abs timers don't set remaining time or restart */
	if (flags == TIMER_ABSTIME)
		return -ERESTARTNOHAND;

	restart->fn = alarm_timer_nsleep_restart;
	restart->nanosleep.clockid = type;
	restart->nanosleep.expires = exp;
	return ret;
}