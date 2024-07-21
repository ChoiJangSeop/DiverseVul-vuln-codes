static int tstats_show(struct seq_file *m, void *v)
{
	struct timespec64 period;
	struct entry *entry;
	unsigned long ms;
	long events = 0;
	ktime_t time;
	int i;

	mutex_lock(&show_mutex);
	/*
	 * If still active then calculate up to now:
	 */
	if (timer_stats_active)
		time_stop = ktime_get();

	time = ktime_sub(time_stop, time_start);

	period = ktime_to_timespec64(time);
	ms = period.tv_nsec / 1000000;

	seq_puts(m, "Timer Stats Version: v0.3\n");
	seq_printf(m, "Sample period: %ld.%03ld s\n", (long)period.tv_sec, ms);
	if (atomic_read(&overflow_count))
		seq_printf(m, "Overflow: %d entries\n", atomic_read(&overflow_count));
	seq_printf(m, "Collection: %s\n", timer_stats_active ? "active" : "inactive");

	for (i = 0; i < nr_entries; i++) {
		entry = entries + i;
		if (entry->flags & TIMER_DEFERRABLE) {
			seq_printf(m, "%4luD, %5d %-16s ",
				entry->count, entry->pid, entry->comm);
		} else {
			seq_printf(m, " %4lu, %5d %-16s ",
				entry->count, entry->pid, entry->comm);
		}

		print_name_offset(m, (unsigned long)entry->start_func);
		seq_puts(m, " (");
		print_name_offset(m, (unsigned long)entry->expire_func);
		seq_puts(m, ")\n");

		events += entry->count;
	}

	ms += period.tv_sec * 1000;
	if (!ms)
		ms = 1;

	if (events && period.tv_sec)
		seq_printf(m, "%ld total events, %ld.%03ld events/sec\n",
			   events, events * 1000 / ms,
			   (events * 1000000 / ms) % 1000);
	else
		seq_printf(m, "%ld total events\n", events);

	mutex_unlock(&show_mutex);

	return 0;
}