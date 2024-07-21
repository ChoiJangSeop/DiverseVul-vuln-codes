static ssize_t last_change_ms_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	struct wakeup_source *ws = dev_get_drvdata(dev);

	return sprintf(buf, "%lld\n", ktime_to_ms(ws->last_time));
}