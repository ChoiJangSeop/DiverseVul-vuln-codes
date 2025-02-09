void hrtimer_start_range_ns(struct hrtimer *timer, ktime_t tim,
			    u64 delta_ns, const enum hrtimer_mode mode)
{
	struct hrtimer_clock_base *base, *new_base;
	unsigned long flags;
	int leftmost;

	base = lock_hrtimer_base(timer, &flags);

	/* Remove an active timer from the queue: */
	remove_hrtimer(timer, base, true);

	if (mode & HRTIMER_MODE_REL)
		tim = ktime_add_safe(tim, base->get_time());

	tim = hrtimer_update_lowres(timer, tim, mode);

	hrtimer_set_expires_range_ns(timer, tim, delta_ns);

	/* Switch the timer base, if necessary: */
	new_base = switch_hrtimer_base(timer, base, mode & HRTIMER_MODE_PINNED);

	timer_stats_hrtimer_set_start_info(timer);

	leftmost = enqueue_hrtimer(timer, new_base);
	if (!leftmost)
		goto unlock;

	if (!hrtimer_is_hres_active(timer)) {
		/*
		 * Kick to reschedule the next tick to handle the new timer
		 * on dynticks target.
		 */
		if (new_base->cpu_base->nohz_active)
			wake_up_nohz_cpu(new_base->cpu_base->cpu);
	} else {
		hrtimer_reprogram(timer, new_base);
	}
unlock:
	unlock_hrtimer_base(timer, &flags);
}