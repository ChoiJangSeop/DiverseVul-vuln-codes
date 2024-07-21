static ssize_t autosuspend_delay_ms_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	if (!dev->power.use_autosuspend)
		return -EIO;
	return sprintf(buf, "%d\n", dev->power.autosuspend_delay);
}