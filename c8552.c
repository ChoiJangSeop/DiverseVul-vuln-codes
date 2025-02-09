static void do_sched_cfs_slack_timer(struct cfs_bandwidth *cfs_b)
{
	u64 runtime = 0, slice = sched_cfs_bandwidth_slice();
	unsigned long flags;
	u64 expires;

	/* confirm we're still not at a refresh boundary */
	raw_spin_lock_irqsave(&cfs_b->lock, flags);
	cfs_b->slack_started = false;
	if (cfs_b->distribute_running) {
		raw_spin_unlock_irqrestore(&cfs_b->lock, flags);
		return;
	}

	if (runtime_refresh_within(cfs_b, min_bandwidth_expiration)) {
		raw_spin_unlock_irqrestore(&cfs_b->lock, flags);
		return;
	}

	if (cfs_b->quota != RUNTIME_INF && cfs_b->runtime > slice)
		runtime = cfs_b->runtime;

	expires = cfs_b->runtime_expires;
	if (runtime)
		cfs_b->distribute_running = 1;

	raw_spin_unlock_irqrestore(&cfs_b->lock, flags);

	if (!runtime)
		return;

	runtime = distribute_cfs_runtime(cfs_b, runtime, expires);

	raw_spin_lock_irqsave(&cfs_b->lock, flags);
	if (expires == cfs_b->runtime_expires)
		lsub_positive(&cfs_b->runtime, runtime);
	cfs_b->distribute_running = 0;
	raw_spin_unlock_irqrestore(&cfs_b->lock, flags);
}