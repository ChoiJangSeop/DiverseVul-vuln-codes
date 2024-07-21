static ssize_t tstats_write(struct file *file, const char __user *buf,
			    size_t count, loff_t *offs)
{
	char ctl[2];

	if (count != 2 || *offs)
		return -EINVAL;

	if (copy_from_user(ctl, buf, count))
		return -EFAULT;

	mutex_lock(&show_mutex);
	switch (ctl[0]) {
	case '0':
		if (timer_stats_active) {
			timer_stats_active = 0;
			time_stop = ktime_get();
			sync_access();
		}
		break;
	case '1':
		if (!timer_stats_active) {
			reset_entries();
			time_start = ktime_get();
			smp_mb();
			timer_stats_active = 1;
		}
		break;
	default:
		count = -EINVAL;
	}
	mutex_unlock(&show_mutex);

	return count;
}