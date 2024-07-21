static ssize_t active_time_ms_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	struct wakeup_source *ws = dev_get_drvdata(dev);
	ktime_t active_time =
		ws->active ? ktime_sub(ktime_get(), ws->last_time) : 0;

	return sprintf(buf, "%lld\n", ktime_to_ms(active_time));
}