static ssize_t prevent_suspend_time_ms_show(struct device *dev,
					    struct device_attribute *attr,
					    char *buf)
{
	struct wakeup_source *ws = dev_get_drvdata(dev);
	ktime_t prevent_sleep_time = ws->prevent_sleep_time;

	if (ws->active && ws->autosleep_enabled) {
		prevent_sleep_time = ktime_add(prevent_sleep_time,
			ktime_sub(ktime_get(), ws->start_prevent_time));
	}
	return sprintf(buf, "%lld\n", ktime_to_ms(prevent_sleep_time));
}