static ssize_t total_time_ms_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	struct wakeup_source *ws = dev_get_drvdata(dev);
	ktime_t active_time;
	ktime_t total_time = ws->total_time;

	if (ws->active) {
		active_time = ktime_sub(ktime_get(), ws->last_time);
		total_time = ktime_add(total_time, active_time);
	}
	return sprintf(buf, "%lld\n", ktime_to_ms(total_time));
}