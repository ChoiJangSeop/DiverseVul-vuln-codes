void timer_stats_update_stats(void *timer, pid_t pid, void *startf,
			      void *timerf, char *comm, u32 tflags)
{
	/*
	 * It doesn't matter which lock we take:
	 */
	raw_spinlock_t *lock;
	struct entry *entry, input;
	unsigned long flags;

	if (likely(!timer_stats_active))
		return;

	lock = &per_cpu(tstats_lookup_lock, raw_smp_processor_id());

	input.timer = timer;
	input.start_func = startf;
	input.expire_func = timerf;
	input.pid = pid;
	input.flags = tflags;

	raw_spin_lock_irqsave(lock, flags);
	if (!timer_stats_active)
		goto out_unlock;

	entry = tstat_lookup(&input, comm);
	if (likely(entry))
		entry->count++;
	else
		atomic_inc(&overflow_count);

 out_unlock:
	raw_spin_unlock_irqrestore(lock, flags);
}