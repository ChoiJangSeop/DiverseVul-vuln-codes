static ssize_t max_time_ms_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct wakeup_source *ws = dev_get_drvdata(dev);
	ktime_t active_time;
	ktime_t max_time = ws->max_time;

	if (ws->active) {
		active_time = ktime_sub(ktime_get(), ws->last_time);
		if (active_time > max_time)
			max_time = active_time;
	}
	return sprintf(buf, "%lld\n", ktime_to_ms(max_time));
}