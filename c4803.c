static void __run_hrtimer(struct hrtimer_cpu_base *cpu_base,
			  struct hrtimer_clock_base *base,
			  struct hrtimer *timer, ktime_t *now)
{
	enum hrtimer_restart (*fn)(struct hrtimer *);
	int restart;

	lockdep_assert_held(&cpu_base->lock);

	debug_deactivate(timer);
	cpu_base->running = timer;

	/*
	 * Separate the ->running assignment from the ->state assignment.
	 *
	 * As with a regular write barrier, this ensures the read side in
	 * hrtimer_active() cannot observe cpu_base->running == NULL &&
	 * timer->state == INACTIVE.
	 */
	raw_write_seqcount_barrier(&cpu_base->seq);

	__remove_hrtimer(timer, base, HRTIMER_STATE_INACTIVE, 0);
	timer_stats_account_hrtimer(timer);
	fn = timer->function;

	/*
	 * Clear the 'is relative' flag for the TIME_LOW_RES case. If the
	 * timer is restarted with a period then it becomes an absolute
	 * timer. If its not restarted it does not matter.
	 */
	if (IS_ENABLED(CONFIG_TIME_LOW_RES))
		timer->is_rel = false;

	/*
	 * Because we run timers from hardirq context, there is no chance
	 * they get migrated to another cpu, therefore its safe to unlock
	 * the timer base.
	 */
	raw_spin_unlock(&cpu_base->lock);
	trace_hrtimer_expire_entry(timer, now);
	restart = fn(timer);
	trace_hrtimer_expire_exit(timer);
	raw_spin_lock(&cpu_base->lock);

	/*
	 * Note: We clear the running state after enqueue_hrtimer and
	 * we do not reprogram the event hardware. Happens either in
	 * hrtimer_start_range_ns() or in hrtimer_interrupt()
	 *
	 * Note: Because we dropped the cpu_base->lock above,
	 * hrtimer_start_range_ns() can have popped in and enqueued the timer
	 * for us already.
	 */
	if (restart != HRTIMER_NORESTART &&
	    !(timer->state & HRTIMER_STATE_ENQUEUED))
		enqueue_hrtimer(timer, base);

	/*
	 * Separate the ->running assignment from the ->state assignment.
	 *
	 * As with a regular write barrier, this ensures the read side in
	 * hrtimer_active() cannot observe cpu_base->running == NULL &&
	 * timer->state == INACTIVE.
	 */
	raw_write_seqcount_barrier(&cpu_base->seq);

	WARN_ON_ONCE(cpu_base->running != timer);
	cpu_base->running = NULL;
}