static void sync_access(void)
{
	unsigned long flags;
	int cpu;

	for_each_online_cpu(cpu) {
		raw_spinlock_t *lock = &per_cpu(tstats_lookup_lock, cpu);

		raw_spin_lock_irqsave(lock, flags);
		/* nothing */
		raw_spin_unlock_irqrestore(lock, flags);
	}
}